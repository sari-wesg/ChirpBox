import itertools

a1 = [[0, 1], [3, 6], [7, 9]]
a2 = [[2, 6], [0, 1]]
a3 = [[2, 6], [0, 1]]

overlap = 0
Required_length = 1
the_max_start = 0xFFFF

for x, y, z in itertools.product(a1, a2, a3):
    max_start = max(x[0], y[0], z[0])
    min_end = min(x[1], y[1], z[1])
    overlap = max(0, min_end-max_start)
    if (overlap >= Required_length):
        the_max_start = min(the_max_start, max_start)
        print(x, y, z)
        print(max_start, min_end)

print(overlap, the_max_start)
