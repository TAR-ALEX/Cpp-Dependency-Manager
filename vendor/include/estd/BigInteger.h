#pragma once

#include <deque>
#include <exception>
#include <iomanip>
#include <map>
#include <sstream>

namespace estd {
    class BigDecimal;
    class BigInteger {
        friend class BigDecimal;

    protected:
        bool isNegative = false;
        std::deque<uint32_t> number; //stored in blocks of 9x base10 digits maxBlock: 999,999,999

        BigInteger(std::deque<uint32_t> d) { number = d; }

        bool isZero() const { return (number.size() == 1) && (number[0] == 0); }

        BigInteger& trimLeadingZeros() {
            while (number.size() > 0) {
                if (number[0] == 0) number.pop_front();
                else
                    break;
            }
            if (number.size() == 0) number.push_front(0);
            return *this;
        }

        BigInteger ninesComplement() const {
            BigInteger result = *this;
            for (size_t i = 0; i < number.size(); i++) { result.number[i] = 999999999 - result.number[i]; }
            return result;
        }

        // (sign is ignored)
        BigInteger unsignedAdd(const BigInteger& left, const BigInteger& right) const {
            uint64_t buffer = 0;

            BigInteger result = std::deque<uint32_t>();

            auto iLeft = left.number.rbegin();
            auto iRight = right.number.rbegin();

            uint64_t vLeft;
            uint64_t vRight;
            while (true) {
                if (iLeft == left.number.rend() && iRight == right.number.rend()) break;

                if (iLeft != left.number.rend()) {
                    vLeft = *iLeft;
                    ++iLeft;
                } else {
                    vLeft = 0;
                }

                if (iRight != right.number.rend()) {
                    vRight = *iRight;
                    ++iRight;
                } else {
                    vRight = 0;
                }

                buffer += vLeft + vRight;
                result.number.push_front(buffer % 1000000000);
                buffer /= 1000000000;
            }
            if (buffer != 0) { result.number.push_front(buffer % 1000000000); }
            return result;
        }

        //PRECONDITION: left MUST be greater than right! (sign is ignored)
        BigInteger unsignedSubtract(const BigInteger& left, const BigInteger& right) const {
            BigInteger result = unsignedAdd(left.ninesComplement(), right).ninesComplement();
            return result.trimLeadingZeros();
        }

        BigInteger unsignedMultiply(const BigInteger& left, const BigInteger& right) const {
            return unsignedMultiplyKaratsuba(left, right);
        }

        BigInteger unsignedMultiplyKaratsuba(const BigInteger left, const BigInteger right) const {
            if (left.number.size() <= 50 || right.number.size() <= 50) return unsignedMultiplySimple(left, right);

            size_t splitSize = left.number.size();
            if (right.number.size() < splitSize) splitSize = right.number.size();
            splitSize /= 2;

            BigInteger high1, low1, high2, low2;

            // [0] [size-splitSize] [size]
            high1.number =
                std::deque<uint32_t>(left.number.begin(), left.number.begin() + left.number.size() - splitSize);
            high2.number =
                std::deque<uint32_t>(right.number.begin(), right.number.begin() + right.number.size() - splitSize);
            low1.number = std::deque<uint32_t>(left.number.begin() + left.number.size() - splitSize, left.number.end());
            low2.number =
                std::deque<uint32_t>(right.number.begin() + right.number.size() - splitSize, right.number.end());

            BigInteger z0 = unsignedMultiplyKaratsuba(low1, low2);
            BigInteger z1 = unsignedMultiplyKaratsuba(unsignedAdd(low1, high1), unsignedAdd(low2, high2));
            BigInteger z2 = unsignedMultiplyKaratsuba(high1, high2);

            z1 = z1 - (z2 + z0);
            for (size_t i = 0; i < splitSize; i++) z1.number.push_back(0);

            for (size_t i = 0; i < splitSize * 2; i++) z2.number.push_back(0);

            return z2 + z1 + z0;
        }

        BigInteger unsignedMultiplySimple(const BigInteger& left, const BigInteger& right) const {
            if (right == 0 || left == 0) return 0;
            uint64_t buffer = 0;

            BigInteger result = nullptr;

            uint64_t vLeft;
            uint64_t vRight;

            int prepush = 0;
            for (auto iRight = right.number.rbegin(); iRight != right.number.rend(); ++iRight) {
                BigInteger tmp = nullptr;
                for (int i = 0; i < prepush; i++) tmp.number.push_front(0);
                prepush++;
                for (auto iLeft = left.number.rbegin(); iLeft != left.number.rend(); ++iLeft) {
                    vLeft = *iLeft;
                    vRight = *iRight;

                    buffer += vLeft * vRight;
                    tmp.number.push_front(buffer % 1000000000);
                    buffer /= 1000000000;
                }
                while (buffer != 0) {
                    tmp.number.push_front(buffer % 1000000000);
                    buffer /= 1000000000;
                }
                result += tmp;
            }
            if (result.number.size() == 0) result.number.push_front(0);
            return result;
        }

        std::pair<BigInteger, BigInteger> unsignedDivide(BigInteger left, BigInteger right) const {
            std::pair<BigInteger, BigInteger> result = {0, 0};

            left.isNegative = false;
            right.isNegative = false;


            if (right == 0) throw std::invalid_argument("Cannot divide by zero");
            if (isMagnitudeLessThan(left, right)) return {0, left};
            if (left == 0) return {0, 0};

            BigInteger leftBound = 0;
            BigInteger rightBound = left;

            BigInteger& target = left;
            while (leftBound <= rightBound) {
                BigInteger guess = (leftBound + rightBound) >> 1;
                BigInteger val = guess * right;

                if (val <= target && (val + right) > target) { return {guess, target - val}; }
                if (target < val) {
                    rightBound = guess - 1;
                } else {
                    leftBound = guess + 1;
                }
            }

            return result;
        }

        BigInteger powerIterative(BigInteger x, BigInteger n) const {
            if (n == 0) return 1;
            if (n == 1) return x;

            BigInteger y = 1;

            while (n > 1) {
                if (n.number[n.number.size() - 1] << 31) { // if is odd
                    y = unsignedMultiply(x, y);
                    x = unsignedMultiply(x, x);
                } else {
                    x = unsignedMultiply(x, x);
                }
                n >>= 1;
            }
            return unsignedMultiply(x, y);
        }

        BigInteger powerRecursive(BigInteger p, std::map<BigInteger, BigInteger>& history) const {
            if (history.count(p)) return history[p];

            BigInteger res = powerRecursive(p >> 1, history) * powerRecursive(p - (p >> 1), history);

            history[p] = res;

            return res;
        }

        bool isMagnitudeLessThan(const BigInteger& left, const BigInteger& right) const {
            if (left.number.size() < right.number.size()) return true;
            if (left.number.size() > right.number.size()) return false;

            for (size_t i = 0; i < left.number.size(); i++) {
                if (left.number[i] < right.number[i]) return true;
                if (left.number[i] > right.number[i]) return false;
            }

            return false;
        }

    public:
        //Constructors
        BigInteger() { this->operator=(int64_t(0)); };
        BigInteger(std::nullptr_t){};
        BigInteger(std::string val) { this->operator=(val); }
        BigInteger(const char* val) { this->operator=(val); }
        // template for integer types
        template <class T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
        BigInteger(T val) {
            this->operator=(val);
        }

        //Assignment operators
        BigInteger& operator=(const char* strNum) {
            *this = std::string(strNum);
            return *this;
        }

        BigInteger& operator=(std::string strNum) {
            number.clear();

            if (strNum.size() == 0) throw std::invalid_argument("Cannot parse BigInteger");

            if (strNum[0] == '-') {
                isNegative = true;
                strNum = strNum.substr(1);
            }

            if (strNum.size() == 0) throw std::invalid_argument("Cannot parse BigInteger");

            for (size_t i = 0; i < strNum.size(); i++) {
                if (strNum[i] < '0' || strNum[i] > '9') throw std::invalid_argument("Cannot parse BigInteger");
            }

            int right = strNum.length();
            int left = right - 9;
            if (left < 0) left = 0;
            while (left != right) {
                unsigned long long val = std::stoull(strNum.substr(left, right - left));
                number.push_front(val);
                right = left;
                left = right - 9;
                if (left < 0) left = 0;
            }
            trimLeadingZeros();
            if (isZero()) { isNegative = false; }
            return *this;
        }

        BigInteger& operator=(intmax_t n) {
            number.clear();

            if (n == 0) {
                number.push_front(0);
                return *this;
            }

            isNegative = false;
            if (n < 0) {
                n = -n;
                isNegative = true;
            }

            while (n != 0) {
                number.push_front(n % 1000000000);
                n /= 1000000000;
            }

            return *this;
        }

        BigInteger& operator=(uintmax_t n) {
            number.clear();

            if (n == 0) {
                number.push_front(0);
                return *this;
            }

            isNegative = false;

            while (n != 0) {
                number.push_front(n % 1000000000);
                n /= 1000000000;
            }

            return *this;
        }
        // template for integer types
        template <class T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
        BigInteger& operator=(T n) {
            if constexpr (std::is_unsigned<T>::value) {
                return operator=(uintmax_t(n));
            } else {
                return operator=(intmax_t(n));
            }
        }

        //Operations
        BigInteger operator+(const BigInteger& right) const {
            const BigInteger& left = *this;
            BigInteger result;

            if (left.isNegative && right.isNegative) {
                result = unsignedAdd(left, right);
                result.isNegative = true;
            } else if (left.isNegative != right.isNegative) {
                if (isMagnitudeLessThan(right, left)) result = unsignedSubtract(left, right);
                else
                    result = unsignedSubtract(right, left);
                if (isMagnitudeLessThan(right, left) && left.isNegative) result.isNegative = true;
                else if (isMagnitudeLessThan(left, right) && right.isNegative)
                    result.isNegative = true;
            } else {
                result = unsignedAdd(left, right);
            }
            return result;
        }

        BigInteger& operator+=(const BigInteger& right) { return (*this) = (*this) + right; }

        BigInteger operator-(BigInteger right) const {
            right.isNegative = !right.isNegative;
            return operator+(right);
        }

        BigInteger& operator-=(const BigInteger& right) { return (*this) = (*this) - right; }

        BigInteger operator*(BigInteger right) const {
            const BigInteger& left = *this;
            BigInteger result;

            result = unsignedMultiply(left, right);
            if (left.isNegative != right.isNegative) result.isNegative = true;

            return result;
        }

        BigInteger& operator*=(const BigInteger& right) { return (*this) = (*this) * right; }

        BigInteger operator/(const BigInteger& right) {
            BigInteger& left = *this;

            auto result = unsignedDivide(left, right);
            if (left.isNegative != right.isNegative) result.first.isNegative = true;

            return result.first;
        }

        BigInteger& operator/=(const BigInteger& right) { return (*this) = (*this) / right; }

        BigInteger operator%(const BigInteger& right) {
            BigInteger& left = *this;

            auto result = unsignedDivide(left, right);
            if (left.isNegative != right.isNegative) result.first.isNegative = true;

            return result.second;
        }

        BigInteger& operator%=(const BigInteger& right) { return (*this) = (*this) % right; }

        BigInteger power(BigInteger p) const {
            if (p == 0) return 0;
            // auto result = powerIterative(*this, p);
            std::map<BigInteger, BigInteger> history = {{1, *this}, {0, 1}};
            auto result = powerRecursive(p, history);
            if ((p.number[p.number.size() - 1] << 31) == 0) result.isNegative = false;
            else
                result.isNegative = this->isNegative;
            return result;
        }

        // BigInteger power(BigDecimal p) const;

        BigInteger operator++(int) {
            BigInteger oldThis = *this;
            *this = *this + 1;
            return oldThis;
        };
        BigInteger& operator++() {
            *this = *this + 1;
            return *this;
        };
        BigInteger operator--(int) {
            BigInteger oldThis = *this;
            *this = *this - 1;
            return oldThis;
        };
        BigInteger& operator--() {
            *this = *this - 1;
            return *this;
        };

        //Comparators
        bool operator==(const BigInteger& right) const {
            const BigInteger& left = *this;

            if (left.isZero() && right.isZero()) return true;
            if (left.number.size() != right.number.size()) return false;
            if (left.isNegative != right.isNegative) return false;
            for (size_t i = 0; i < left.number.size(); i++) {
                if (left.number[i] != right.number[i]) return false;
            }
            return true;
        }

        bool operator!=(const BigInteger& right) const { return !(operator==(right)); }

        bool operator>(const BigInteger& right) const {
            const BigInteger& left = *this;
            return right < left;
        }

        bool operator>=(const BigInteger& right) const {
            const BigInteger& left = *this;
            return !(left < right);
        }

        bool operator<(const BigInteger& right) const {
            const BigInteger& left = *this;
            if (left == right) return false;
            if (left.isNegative && !right.isNegative) return true;
            else if (!left.isNegative && right.isNegative)
                return false;
            else if (left.isNegative && right.isNegative)
                return isMagnitudeLessThan(right, left);
            return isMagnitudeLessThan(left, right);
        }

        bool operator<=(const BigInteger& right) const {
            const BigInteger& left = *this;
            return !(left > right);
        }

        //TODO: implement power function and optimize this using divide and conq.
        BigInteger& operator<<=(const BigInteger& right) {
            BigInteger& result = *this;
            for (BigInteger i = 0; i < right; i++) { result = unsignedMultiply(result, BigInteger(2)); }
            if (result.number.size() == 0) result.number.push_front(0);
            return result;
        }

        //TODO: implement power function and optimize this using divide and conq.
        BigInteger& operator>>=(const BigInteger& right) {
            BigInteger& result = *this;
            for (BigInteger i = 0; i < right; i++) {
                //essentially we multiply by 5/10 using clever tricks
                result = unsignedMultiply(result, BigInteger(5 * (1000000000 / 10)));
                result.number.pop_back(); // divides by 1000000000
            }
            if (result.number.size() == 0) result.number.push_front(0);
            return result;
        }

        BigInteger operator<<(const BigInteger& right) const {
            BigInteger result = *this;
            result <<= right;
            return result;
        }

        BigInteger operator>>(const BigInteger& right) const {
            BigInteger result = *this;
            result >>= right;
            return result;
        }

        friend std::ostream& operator<<(std::ostream& out, const BigInteger& right) { return out << right.toString(); }
        friend std::istream& operator>>(std::istream& in, BigInteger& right); //TODO

        uintmax_t toUint() const {
            uintmax_t result = 0;
            if (number.size() >= 1) result += number[number.size() - 1];
            if (number.size() >= 2) result += number[number.size() - 2] * 1000000000;
            if (number.size() >= 3) result += number[number.size() - 3] * 1000000000 * 1000000000;
            return result;
        }
        intmax_t toInt() const {
            uintmax_t result = toUint();
            result <<= 1;
            result >>= 1;
            if (isNegative) return -intmax_t(result);
            return intmax_t(result);
        }

        // template for integer types
        template <class T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
        explicit operator T() {
            if constexpr (std::is_unsigned<T>::value) {
                return toUint();
            } else {
                return toInt();
            }
        }

        std::string toString() const {
            if (number.size() == 0) return "nan";
            std::stringstream ss;
            if (isNegative) ss << "-";
            for (auto token : number) {
                ss << token;
                ss << std::setfill('0') << std::setw(9);
            }
            return ss.str();
        }
        size_t getNumDigits() {
            if (number.size() < 1) return 0;
            size_t result = 0;
            result = (number.size() - 1) * 9;
            uint64_t tmp = number[0];
            for (int i = 0; i < 9; i++) {
                if (tmp != 0) result++;
                tmp /= 10;
            }
            if (result <= 0) result = 1;
            return result;
        }

        operator BigDecimal();

        // TODO:
        // DEFINE_BIN_OP(^) // makes no sense for long ints, will just be all 1s  if we assume leading zeros
        // DEFINE_BIN_OP(&) // can be done, but hard for the stored format, will be slow
        // DEFINE_BIN_OP(|) // can be done, but hard for the stored format, will be slow
        // DEFINE_UNARY_OP(~) // makes no sense for long ints, will just be all 1s if we assume leading zeros
        // DEFINE_BIN_OP(^=) // makes no sense for long ints, will just be all 1s  if we assume leading zeros
        // DEFINE_BIN_OP(&=) // can be done, but hard for the stored format, will be slow
        // DEFINE_BIN_OP(|=) // can be done, but hard for the stored format, will be slow
    };
    typedef BigInteger BigInt;
} // namespace estd

#include <estd/BigDecimal.h> // correct order