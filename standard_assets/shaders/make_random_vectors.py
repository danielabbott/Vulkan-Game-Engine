import random
from math import sqrt

positions = []
for i in range(16):
	p = [random.random()*2-1, random.random()*2-1]
	length = sqrt(p[0]*p[0] + p[1]*p[1])
	new_length = random.random()
	new_length = (new_length*new_length)*0.9 + 0.1
	multiplier = new_length / length
	p[0] *= multiplier
	p[1] *= multiplier


	print('vec2(' + str(p[0])  + ', ' + str(p[1]) + '),')
	#print('(' + str(p[0])  + ', ' + str(p[1]) + ')')