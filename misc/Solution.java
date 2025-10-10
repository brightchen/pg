public class Solution {
    private static Map<Integer, int[][]> matrix = new HashMap<>();

    public static void main(String[] args) {
        System.out.println(f(3));   // Should print 2
        System.out.println(f(5));   // Should print 5
        System.out.println(f(10));  // Should print 55
    }

    static int f(int n) {
        if (n == 0) return 0;
        if (n == 1) return 1;
        int[][] m = power(n);
        return m[0][1];
    }

    static int[][] power(int n) {
        if (n == 1) {
            return new int[][]{{1, 1}, {1, 0}};
        }

        if (matrix.containsKey(n)) {
            return matrix.get(n);
        }

        int[][] result;
        if (n % 2 == 0) {
            // For even n: M^n = M^(n/2) * M^(n/2)
            int[][] half = power(n / 2);
            result = mul(half, half);
        } else {
            // For odd n: M^n = M^(n-1) * M^1
            result = mul(power(n - 1), power(1));
        }

        matrix.put(n, result);
        return result;
    }

    static int[][] mul(int[][] m1, int[][] m2) {
        int[][] nm = new int[2][2];
        nm[0][0] = m1[0][0] * m2[0][0] + m1[0][1] * m2[1][0];
        nm[0][1] = m1[0][0] * m2[0][1] + m1[0][1] * m2[1][1];
        nm[1][0] = m1[1][0] * m2[0][0] + m1[1][1] * m2[1][0];
        nm[1][1] = m1[1][0] * m2[0][1] + m1[1][1] * m2[1][1];
        return nm;
    }
}
