import os
import argparse
import numpy as np


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rows', '-r', type=int)
    parser.add_argument('--cols', '-c', type=int)
    parser.add_argument('--n-files', '-n', type=int)
    parser.add_argument('--dir', '-d', type=str)
    args = parser.parse_args()

    shape = (args.rows, args.cols)

    os.makedirs(args.dir, exist_ok=True)

    mat_sum = np.zeros(shape)
    for i in range(1, args.n_files+1):
        mat_summand = np.random.randint(0, 1000, shape, dtype=int)
        mat_sum += mat_summand
        np.savetxt(os.path.join(args.dir, 'matrix{}.txt'.format(i)), mat_summand, fmt='%i')
    np.savetxt(os.path.join(args.dir, 'target.txt'), mat_sum, fmt='%i')


if __name__ == '__main__':
    main()
