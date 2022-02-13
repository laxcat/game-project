struct TwoIntKey {
    int16_t a;
    int16_t b;

    bool operator==(TwoIntKey const & other) const {
        return (a == other.a && b == other.b);
    }
};

namespace std {
    template <>
    struct hash<TwoIntKey> {
        size_t operator()(TwoIntKey const & k) const {
            return (size_t)(k.a << 16 | k.b);
        }
    };
}