import os
import argparse
import numpy as np


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--size', '-N', type=int)
    parser.add_argument('--n-files', '-n', type=int)
    parser.add_argument('--dir', '-d', type=str)
    args = parser.parse_args()

    shape = (args.size, args.size)

    os.makedirs(args.dir, exist_ok=True)

    mat_product = np.eye(args.size, args.size)
    for i in range(1, args.n_files+1):
        mat_multiplier = np.random.randint(-10, 10, shape, dtype=int)
        mat_product = np.matmul(mat_product, mat_multiplier)
        np.savetxt(os.path.join(args.dir, 'matrix{}.txt'.format(i)), mat_multiplier, fmt='%i')
    np.savetxt(os.path.join(args.dir, 'target.txt'), mat_product, fmt='%i')


if __name__ == '__main__':
    main()
