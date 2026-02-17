#include "helloworld.grpc.pb.h"
#include "helloworld.pb.h"
#include "route_guide.grpc.pb.h"
#include "route_guide.pb.h"
#include "tutorial.grpc.pb.h"
#include "tutorial.pb.h"

#include <grpc/grpc.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/server_builder.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <regex>
#include <thread>

using namespace std;
using namespace chrono;

namespace routeguide {

// A simple parser for the json db file. It requires the db file to have the
// exact form of [{"location":{"latitude":123,"longitude":456},"name":
// "the name can be empty"},{ ... }...
class Parser {
public:
  explicit Parser(const std::string &db) : db_(db) {
    if (!Match("[")) {
      SetFailedAndReturnFalse();
    }
  }

  bool Finished() { return current_ >= db_.size(); }

  bool TryParseOne(Feature *feature) {
    if (failed_ || Finished() || !Match("{")) {
      return SetFailedAndReturnFalse();
    }
    if (!Match(location_) || !Match("{") || !Match(latitude_)) {
      return SetFailedAndReturnFalse();
    }
    long temp = 0;
    ReadLong(&temp);
    feature->mutable_location()->set_latitude(temp);
    if (!Match(",") || !Match(longitude_)) {
      return SetFailedAndReturnFalse();
    }
    ReadLong(&temp);
    feature->mutable_location()->set_longitude(temp);
    if (!Match("},") || !Match(name_) || !Match("\"")) {
      return SetFailedAndReturnFalse();
    }
    size_t name_start = current_;
    while (current_ != db_.size() && db_[current_++] != '"') {
    }
    if (current_ == db_.size()) {
      return SetFailedAndReturnFalse();
    }
    feature->set_name(db_.substr(name_start, current_ - name_start - 1));
    if (!Match("},")) {
      if (db_[current_ - 1] == ']' && current_ == db_.size()) {
        return true;
      }
      return SetFailedAndReturnFalse();
    }
    return true;
  }

private:
  bool SetFailedAndReturnFalse() {
    failed_ = true;
    return false;
  }

  bool Match(const std::string &prefix) {
    bool eq = db_.substr(current_, prefix.size()) == prefix;
    current_ += prefix.size();
    return eq;
  }

  void ReadLong(long *l) {
    size_t start = current_;
    while (current_ != db_.size() && db_[current_] != ',' &&
           db_[current_] != '}') {
      current_++;
    }
    // It will throw an exception if fails.
    *l = std::stol(db_.substr(start, current_ - start));
  }

  bool failed_ = false;
  std::string db_;
  size_t current_ = 0;
  const std::string location_ = "\"location\":";
  const std::string latitude_ = "\"latitude\":";
  const std::string longitude_ = "\"longitude\":";
  const std::string name_ = "\"name\":";
};

// Minifies a JSON string by removing all whitespace characters outside of
// strings.
std::string MinifyJson(const std::string &json) {
  std::regex whitespaceOutsideQuotes(R"(\s+(?=(?:(?:[^"]*"){2})*[^"]*$))");
  // Replace all matches with an empty string
  return std::regex_replace(json, whitespaceOutsideQuotes, "");
}

void ParseDb(const std::string &db, std::vector<Feature> *feature_list) {
  feature_list->clear();
  std::string db_content(MinifyJson(db));

  Parser parser(db_content);
  Feature feature;
  while (!parser.Finished()) {
    feature_list->push_back(Feature());
    if (!parser.TryParseOne(&feature_list->back())) {
      std::cerr << "Error parsing the db file\n";
      feature_list->clear();
      break;
    }
  }
  std::cout << "DB parsed, loaded " << feature_list->size() << " features.\n";
}

float ConvertToRadians(float num) { return num * 3.1415926 / 180; }

// The formula is based on http://mathforum.org/library/drmath/view/51879.html
float GetDistance(const Point &start, const Point &end) {
  const float kCoordFactor = 10000000.0;
  float lat_1 = start.latitude() / kCoordFactor;
  float lat_2 = end.latitude() / kCoordFactor;
  float lon_1 = start.longitude() / kCoordFactor;
  float lon_2 = end.longitude() / kCoordFactor;
  float lat_rad_1 = ConvertToRadians(lat_1);
  float lat_rad_2 = ConvertToRadians(lat_2);
  float delta_lat_rad = ConvertToRadians(lat_2 - lat_1);
  float delta_lon_rad = ConvertToRadians(lon_2 - lon_1);

  float a = pow(sin(delta_lat_rad / 2), 2) +
            cos(lat_rad_1) * cos(lat_rad_2) * pow(sin(delta_lon_rad / 2), 2);
  float c = 2 * atan2(sqrt(a), sqrt(1 - a));
  int R = 6371000; // metres

  return R * c;
}

std::string GetFeatureName(const Point &point,
                           const std::vector<Feature> &feature_list) {
  for (const Feature &f : feature_list) {
    if (f.location().latitude() == point.latitude() &&
        f.location().longitude() == point.longitude()) {
      return f.name();
    }
  }
  return "";
}

class RouteGuideImpl final : public RouteGuide::Service {
public:
  RouteGuideImpl(const std::string &db) { ParseDb(db, &feature_list_); }

  grpc::Status GetFeature(grpc::ServerContext *context, const Point *point,
                          Feature *feature) override {
    feature->set_name(GetFeatureName(*point, feature_list_));
    feature->mutable_location()->CopyFrom(*point);
    return grpc::Status::OK;
  }

  grpc::Status ListFeatures(grpc::ServerContext *context,
                            const routeguide::Rectangle *rectangle,
                            grpc::ServerWriter<Feature> *writer) override {
    auto lo = rectangle->lo();
    auto hi = rectangle->hi();
    long left = (std::min)(lo.longitude(), hi.longitude());
    long right = (std::max)(lo.longitude(), hi.longitude());
    long top = (std::max)(lo.latitude(), hi.latitude());
    long bottom = (std::min)(lo.latitude(), hi.latitude());
    for (const Feature &f : feature_list_) {
      if (f.location().longitude() >= left &&
          f.location().longitude() <= right &&
          f.location().latitude() >= bottom && f.location().latitude() <= top) {
        writer->Write(f);
      }
    }
    return grpc::Status::OK;
  }

  grpc::Status RecordRoute(grpc::ServerContext *context,
                           grpc::ServerReader<Point> *reader,
                           RouteSummary *summary) override {
    Point point;
    int point_count = 0;
    int feature_count = 0;
    float distance = 0.0;
    Point previous;

    system_clock::time_point start_time = system_clock::now();
    while (reader->Read(&point)) {
      point_count++;
      if (!GetFeatureName(point, feature_list_).empty()) {
        feature_count++;
      }
      if (point_count != 1) {
        distance += GetDistance(previous, point);
      }
      previous = point;
    }
    system_clock::time_point end_time = system_clock::now();
    summary->set_point_count(point_count);
    summary->set_feature_count(feature_count);
    summary->set_distance(static_cast<long>(distance));
    auto secs =
        std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
    summary->set_elapsed_time(secs.count());

    return grpc::Status::OK;
  }

  grpc::Status
  RouteChat(grpc::ServerContext *context,
            grpc::ServerReaderWriter<RouteNote, RouteNote> *stream) override {
    RouteNote note;
    while (stream->Read(&note)) {
      std::unique_lock<std::mutex> lock(mu_);
      for (const RouteNote &n : received_notes_) {
        if (n.location().latitude() == note.location().latitude() &&
            n.location().longitude() == note.location().longitude()) {
          stream->Write(n);
        }
      }
      received_notes_.push_back(note);
    }

    return grpc::Status::OK;
  }

private:
  std::vector<Feature> feature_list_;
  std::mutex mu_;
  std::vector<RouteNote> received_notes_;
};

Point MakePoint(long latitude, long longitude) {
  Point p;
  p.set_latitude(latitude);
  p.set_longitude(longitude);
  return p;
}

Feature MakeFeature(const std::string &name, long latitude, long longitude) {
  Feature f;
  f.set_name(name);
  f.mutable_location()->CopyFrom(MakePoint(latitude, longitude));
  return f;
}

RouteNote MakeRouteNote(const std::string &message, long latitude,
                        long longitude) {
  RouteNote n;
  n.set_message(message);
  n.mutable_location()->CopyFrom(MakePoint(latitude, longitude));
  return n;
}

class RouteGuideClient {
public:
  RouteGuideClient(std::shared_ptr<grpc::ChannelInterface> channel,
                   const std::string &db)
      : stub_(RouteGuide::NewStub(channel)) {
    ParseDb(db, &feature_list_);
  }

  void GetFeature() {
    Point point;
    Feature feature;
    point = MakePoint(409146138, -746188906);
    GetOneFeature(point, &feature);
    point = MakePoint(0, 0);
    GetOneFeature(point, &feature);
  }

  void ListFeatures() {
    routeguide::Rectangle rect;
    Feature feature;
    grpc::ClientContext context;

    rect.mutable_lo()->set_latitude(400000000);
    rect.mutable_lo()->set_longitude(-750000000);
    rect.mutable_hi()->set_latitude(420000000);
    rect.mutable_hi()->set_longitude(-730000000);
    std::cout << "Looking for features between 40, -75 and 42, -73"
              << std::endl;

    std::unique_ptr<grpc::ClientReader<Feature>> reader(
        stub_->ListFeatures(&context, rect));
    while (reader->Read(&feature)) {
      std::cout << "Found feature called " << feature.name() << " at "
                << feature.location().latitude() / kCoordFactor_ << ", "
                << feature.location().longitude() / kCoordFactor_ << std::endl;
    }
    grpc::Status status = reader->Finish();
    if (status.ok()) {
      std::cout << "ListFeatures rpc succeeded." << std::endl;
    } else {
      std::cout << "ListFeatures rpc failed." << std::endl;
    }
  }

  void RecordRoute() {
    Point point;
    RouteSummary stats;
    grpc::ClientContext context;
    const int kPoints = 10;
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

    std::default_random_engine generator(seed);
    std::uniform_int_distribution<int> feature_distribution(
        0, feature_list_.size() - 1);
    std::uniform_int_distribution<int> delay_distribution(500, 1500);

    std::unique_ptr<grpc::ClientWriter<Point>> writer(
        stub_->RecordRoute(&context, &stats));
    for (int i = 0; i < kPoints; i++) {
      const Feature &f = feature_list_[feature_distribution(generator)];
      std::cout << "Visiting point " << f.location().latitude() / kCoordFactor_
                << ", " << f.location().longitude() / kCoordFactor_
                << std::endl;
      if (!writer->Write(f.location())) {
        // Broken stream.
        break;
      }
      std::this_thread::sleep_for(
          std::chrono::milliseconds(delay_distribution(generator)));
    }
    writer->WritesDone();
    grpc::Status status = writer->Finish();
    if (status.ok()) {
      std::cout << "Finished trip with " << stats.point_count() << " points\n"
                << "Passed " << stats.feature_count() << " features\n"
                << "Travelled " << stats.distance() << " meters\n"
                << "It took " << stats.elapsed_time() << " seconds"
                << std::endl;
    } else {
      std::cout << "RecordRoute rpc failed." << std::endl;
    }
  }

  void RouteChat() {
    grpc::ClientContext context;

    std::shared_ptr<grpc::ClientReaderWriter<RouteNote, RouteNote>> stream(
        stub_->RouteChat(&context));

    std::thread writer([stream]() {
      std::vector<RouteNote> notes{MakeRouteNote("First message", 0, 0),
                                   MakeRouteNote("Second message", 0, 1),
                                   MakeRouteNote("Third message", 1, 0),
                                   MakeRouteNote("Fourth message", 0, 0)};
      for (const RouteNote &note : notes) {
        std::cout << "Sending message " << note.message() << " at "
                  << note.location().latitude() << ", "
                  << note.location().longitude() << std::endl;
        stream->Write(note);
      }
      stream->WritesDone();
    });

    RouteNote server_note;
    while (stream->Read(&server_note)) {
      std::cout << "Got message " << server_note.message() << " at "
                << server_note.location().latitude() << ", "
                << server_note.location().longitude() << std::endl;
    }
    writer.join();
    grpc::Status status = stream->Finish();
    if (!status.ok()) {
      std::cout << "RouteChat rpc failed." << std::endl;
    }
  }

private:
  bool GetOneFeature(const Point &point, Feature *feature) {
    grpc::ClientContext context;
    grpc::Status status = stub_->GetFeature(&context, point, feature);
    if (!status.ok()) {
      std::cout << "GetFeature rpc failed." << std::endl;
      return false;
    }
    if (!feature->has_location()) {
      std::cout << "Server returns incomplete feature." << std::endl;
      return false;
    }
    if (feature->name().empty()) {
      std::cout << "Found no feature at "
                << feature->location().latitude() / kCoordFactor_ << ", "
                << feature->location().longitude() / kCoordFactor_ << std::endl;
    } else {
      std::cout << "Found feature called " << feature->name() << " at "
                << feature->location().latitude() / kCoordFactor_ << ", "
                << feature->location().longitude() / kCoordFactor_ << std::endl;
    }
    return true;
  }

  const float kCoordFactor_ = 10000000.0;
  std::unique_ptr<RouteGuide::Stub> stub_;
  std::vector<Feature> feature_list_;
};

} // namespace routeguide

void RunServer(const std::string &db_path) {
  std::string server_address("0.0.0.0:50051");
  routeguide::RouteGuideImpl service(db_path);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

std::string ReadFile(const std::string &path) {
  std::ifstream in(path);
  if (!in)
    throw std::runtime_error("Cannot open " + path);
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

int main(int argc, char **argv) {
  // Verifies mismatch of protobuf versions
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  // Optional: Arena for better heap allocation incase a lot of messages are
  // created
  // google::protobuf::Arena arena;

  const std::string db_path = "route_guide_db.json";

  std::thread server{RunServer, ReadFile(db_path)};

  routeguide::RouteGuideClient guide(
      grpc::CreateChannel("localhost:50051",
                          grpc::InsecureChannelCredentials()),
      ReadFile(db_path));
  std::array<std::function<void()>, 4> functions{
      [&guide] { guide.GetFeature(); },
      [&guide] { guide.ListFeatures(); },
      [&guide] { guide.RecordRoute(); },
      [&guide] { guide.RouteChat(); },
  };

  std::thread client{[&functions] {
    int i = 0;
    while (i < 100) {
      if (i % 4 == 1) {
        i++;
        continue;
      }
      functions[i % 4]();
      i++;
      std::this_thread::sleep_for(5000ms);
    }
  }};

  client.join();
  server.join();

  // Optional:  Delete all global objects allocated by libprotobuf.
  google::protobuf::ShutdownProtobufLibrary();
}
