func main() {
    printf("%u\n", calculate(4, 6, 9));
}

func calculate(int x, int y, int z) {
    return x * (x + y) - z;
}
