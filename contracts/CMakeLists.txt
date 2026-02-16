set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
set(PROTO_FILES
  ${PROTO_DIR}/common.proto
  ${PROTO_DIR}/market_data.proto
  ${PROTO_DIR}/strategy_signal.proto
  ${PROTO_DIR}/order.proto
  ${PROTO_DIR}/execution.proto
  ${PROTO_DIR}/risk.proto
  ${PROTO_DIR}/gateway.proto
)

add_library(contracts_proto STATIC)
target_link_libraries(contracts_proto PUBLIC protobuf::libprotobuf)

protobuf_generate(
  TARGET contracts_proto
  LANGUAGE cpp
  PROTOS ${PROTO_FILES}
  IMPORT_DIRS ${PROTO_DIR}
  PROTOC_OUT_DIR ${CMAKE_BINARY_DIR}/generated
)

target_include_directories(contracts_proto
  PUBLIC
    ${CMAKE_BINARY_DIR}/generated
    ${PROTO_DIR}
)

# ---- important part: explicit Abseil link ----
if(TARGET absl::check AND TARGET absl::log)
  target_link_libraries(contracts_proto PUBLIC absl::check absl::log)
elseif(TARGET absl::log_internal_check_op)
  # fallback for some package layouts
  target_link_libraries(contracts_proto PUBLIC
    absl::log_internal_check_op
    absl::log_internal_message
    absl::base
    absl::strings
  )
else()
  message(FATAL_ERROR "Abseil targets not found. Install abseil-cpp dev package.")
endif()

add_library(club::contracts ALIAS contracts_proto)

# Optional python generation target (unchanged)
set(PY_PROTO_OUT ${CMAKE_SOURCE_DIR}/research/generated)
add_custom_target(proto_py
  COMMAND ${CMAKE_COMMAND} -E make_directory ${PY_PROTO_OUT}
  COMMAND ${Protobuf_PROTOC_EXECUTABLE}
          --proto_path=${PROTO_DIR}
          --python_out=${PY_PROTO_OUT}
          ${PROTO_FILES}
  DEPENDS ${PROTO_FILES}
  COMMENT "Generating Python protobuf modules in research/generated"
)
