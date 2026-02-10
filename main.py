import my_app as ma

print(ma.add0(1, 0))
print(ma.add1(1, ma.the_answer))
print(ma.add2())

p = ma.Pet("Loai")
print(p)
print(p.getName())

p.setName("Aris")
print(p.getName())

p.setName(ma.what)
print(p.getName())

p.name = "Hedi"
print(p.getName())
