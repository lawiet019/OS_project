import sys
import math
class Rand48(object):
    def __init__(self, seed):
        self.n = seed
    def seed(self, seed):
        self.n = seed
    def srand(self, seed):
        self.n = (seed << 16) + 0x330e
    def next(self):
        self.n = (25214903917 * self.n + 11) & (2**48 - 1)
        return self.n
    def drand(self):
        return self.next() / 2**48
    def lrand(self):
        return self.next() >> 17
    def mrand(self):
        n = self.next() >> 16
        if n & (1 << 31):
            n -= 1 << 32
        return n
def getNextRandom(r,upper,lamb):
    # set a value greater than upper to enter the while loop
    new_value = upper +1
    while math.ceil(new_value) > upper:
        num = r.rand()
        new_value = - math.log(r) / lamb

    return new_value

def main():
    seed = sys.argv[1]
    lamb = sys.argv[2]
    upper = sys.argv[3]
    num_of_pro = sys.argv[4]
    time_switch = sys.argv[5]
    alpha = sys.argv[6]
    t_slice= sys.argv[7]
    rr_add = sys.argv[8]
    # generate the random number

    r = Rand48(0)
    #set the seed of random number generator
    r.srand(seed)



if __name__ == '__main__':
    main()





