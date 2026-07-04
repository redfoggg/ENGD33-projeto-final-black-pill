from random import random
from math import pi

size = 100


def generate_random_array(title, max):
    txt = f"float {title} [{size}] = " + "{\n"
    for _ in range(size):
        value = random()*max
        txt += f"\t{value}"
        if (_ < size-1):
            txt += ",\n"
    txt += "\n};\n\n"
    return txt


arrays = [
    {
        "title": "currentVectorX",
        "max": 4
    },
    {
        "title": "currentVectorY",
        "max": 4
    },
    {
        "title": "currentVectorZ",
        "max": 4
    },
    {
        "title": "angularVelVectorX",
        "max": 10*pi
    },
    {
        "title": "angularVelVectorY",
        "max": 10*pi
    },
    {
        "title": "angularVelVectorZ",
        "max": 10*pi
    },
    {
        "title": "positionVectorX",
        "max": 1000
    },
    {
        "title": "positionVectorY",
        "max": 1000
    },
    {
        "title": "positionVectorZ",
        "max": 1000
    },
]

with open("../Core/Src/lookuptable.c", 'w') as f:
    for array in arrays:
        f.writelines(generate_random_array(array["title"], array["max"]))
