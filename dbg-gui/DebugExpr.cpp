#include "DebugExpr.h"

#include "ps3emu/ppu/PPUThread.h"
#include "ps3emu/spu/SPUThread.h"
#include "ps3emu/MainMemory.h"
#include "ps3emu/utils.h"
#include "ps3emu/state.h"
#include <memory>
#include <boost/regex.hpp>
#include <boost/endian/arithmetic.hpp>

using namespace boost;

enum class TokenType {
    LeftSquareBracket,
    RightSquareBracket,
    LeftParen,
    RightParen,
    Equals,
    Assign,
    ID,
    Hex,
    Int,
    Minus,
    Plus,
    Mul,
    Div
};

std::string printTokenType(TokenType type) {
#define CASE(t) case TokenType::t: return #t;
    switch (type) {
        CASE(LeftSquareBracket)
        CASE(RightSquareBracket)
        CASE(LeftParen)
        CASE(RightParen)
        CASE(Equals)
        CASE(Assign)
        CASE(ID)
        CASE(Hex)
        CASE(Int)
        CASE(Minus)
        CASE(Plus)
        CASE(Mul)
        CASE(Div)
    }
#undef CASE
    throw std::runtime_error("");
}

class Token {
    TokenType _type;
    std::string _val;
    int _start;
    int _len;
public:
    Token(TokenType type, std::string val, int start, int len)
        : _type(type), _val(val), _start(start), _len(len) { }
        
    std::string val() const {
        return _val;
    }
    
    TokenType type() const {
        return _type;
    }
    
    int len() {
        return _len;
    }
};

Token makeToken(std::string const& text, int start) {
    std::vector<std::tuple<regex, TokenType>> rules {
        std::make_tuple(regex("\\["), TokenType::LeftSquareBracket),
        std::make_tuple(regex("\\]"), TokenType::RightSquareBracket),
        std::make_tuple(regex("\\("), TokenType::LeftParen),
        std::make_tuple(regex("\\)"), TokenType::RightParen),
        std::make_tuple(regex("=="), TokenType::Equals),
        std::make_tuple(regex("="), TokenType::Assign),
        std::make_tuple(regex("#[0-9a-fA-F]+"), TokenType::Hex),
        std::make_tuple(regex("[0-9]+"), TokenType::Int),
        std::make_tuple(regex("\\+"), TokenType::Plus),
        std::make_tuple(regex("-"), TokenType::Minus),
        std::make_tuple(regex("\\*"), TokenType::Mul),
        std::make_tuple(regex("/"), TokenType::Div),
        std::make_tuple(regex("[_a-zA-Z][a-zA-Z0-9]*"), TokenType::ID)
    };
    for (auto& r : rules) {
        regex rx;
        TokenType type;
        std::tie(rx, type) = r;
        smatch m;
        if (regex_search(begin(text) + start, end(text), m, rx,
            regex_constants::match_continuous)) {
            auto len = std::distance(m[0].first, m[0].second);
            return Token(type, text.substr(start, len), start, len);
        }
    }
    throw std::runtime_error("bad token");
}

std::vector<Token> tokenize(std::string const& text) {
    std::vector<Token> tokens;
    for (auto i = 0u; i < text.size();) {
        if (text[i] == ' ' || text[i] == '\t') {
            i += 1;
        } else {
            auto token = makeToken(text, i);
            i += token.len();
            tokens.push_back(token);
        }
    }
    return tokens;
}

class Expr {
public:
    virtual uint64_t eval(PPUThread* th) = 0;
    virtual uint64_t eval(SPUThread* th) = 0;
    ~Expr() { }
};

class BinaryOpExpr : public Expr {
    TokenType type;
    Expr* left;
    Expr* right;
    
    uint64_t evalType(uint64_t l, uint64_t r) {
        switch (type) {
            case TokenType::Plus: return l + r;
            case TokenType::Minus: return l - r;
            case TokenType::Mul: return l * r;
            case TokenType::Div: return l / r;
            default: throw std::runtime_error("");
        }
    }
    
public:
    BinaryOpExpr(TokenType type, Expr* left, Expr* right)
        : type(type), left(left), right(right) { }
    
    uint64_t eval(PPUThread* th) override {
        auto l = left->eval(th);
        auto r = right->eval(th);
        return evalType(l, r);
    }
    
    uint64_t eval(SPUThread* th) override {
        auto l = left->eval(th);
        auto r = right->eval(th);
        return evalType(l, r);
    }
};

class IDExpr : public Expr {
public:
    IDExpr(std::string name) : name(name) { }
    std::string name;
    
    uint64_t eval(PPUThread* th) override {
        regex rxgpr("r([0-9]+)");
        smatch m;
        if (regex_match(name, m, rxgpr)) {
            auto n = std::stoul(m[1]);
            if (n <= 31) {
                return th->getGPR(n);
            }
        }
        if (name == "nip") {
            return th->getNIP();
        }
        if (name == "lr") {
            return th->getLR();
        }
        throw std::runtime_error("");
    }
    
    uint64_t eval(SPUThread* th) override {
        regex rxgpr("r([0-9]+)(b|h|w|d)([0-9]+)");
        smatch m;
        if (regex_match(name, m, rxgpr)) {
            auto n = std::stoul(m[1]);
            auto type = m[2];
            auto part = std::stoul(m[3]);
            if (n <= 127) {
                if (type == "b" && part <= 16)
                    return th->r(n).b(part);
                if (type == "h" && part <= 8)
                    return th->r(n).hw(part);
                if (type == "w" && part <= 4)
                    return th->r(n).w(part);
                if (type == "d" && part <= 2)
                    return th->r(n).dw(part);
            }
        }
        if (name == "nip") {
            return th->getNip();
        }
        throw std::runtime_error("");
    }
};

class NumExpr : public Expr {
public:
    NumExpr(int n) : n(n) { }
    int n;
    uint64_t eval(PPUThread* th) override {
        return n;
    }
    
    uint64_t eval(SPUThread* th) override {
        return n;
    }
};

class MemExpr : public Expr {
public:
    MemExpr(Expr* expr) : expr(expr) { }
    Expr* expr;
    uint64_t eval(PPUThread* th) override {
        return g_state.mm->load<4>(expr->eval(th));
    }
    
    uint64_t eval(SPUThread* th) override {
        return *(boost::endian::big_uint32_t*)th->ptr(expr->eval(th));
    }
};

class ParsingContext {
    std::vector<std::unique_ptr<Expr>> _exprs;
public:
    template <typename E, typename... P>
    E* make(P&&... args) {
        auto expr = std::make_unique<E>(args...);
        auto ptr = expr.get();
        _exprs.emplace_back(std::move(expr));
        return ptr;
    }
};

/*

    factor    = id | num | hex | parens | mem
    mem       = '[' expr ']'
    parens    = '(' expr ')'
    expr      = term { ('+' | '-') term }
    term      = factor { ('*' | '/') factor }

*/

class Parser {
    ParsingContext _context;
    const std::vector<Token>* _tokens;
    int _pos;
    
public:
    Parser(const std::vector<Token>* tokens, const std::string* text)
        : _tokens(tokens), _pos(0) { }
    
    Token const& cur() {
        return (*_tokens)[_pos];
    }
    
    TokenType type() {
        return cur().type();
    }
    
    Token const& curnext() {
        auto& t = cur();
        next();
        return t;
    }
    
    void next() {
        _pos++;
    }
    
    void error(std::string expected) {
        auto token = ssnprintf("%s ('%s')", printTokenType(type()), cur().val());
        auto msg = ssnprintf("expected %s but got %s at pos %d", expected, token, _pos);
        throw std::runtime_error(msg);
    }  
    
    template <typename F>
    Expr* expect(F rule) {
        auto pos = _pos;
        auto r = (this->*rule)();
        if (!r) {
            _pos = pos;
            error("");
        }
        return r;
    }
    
    void expect(TokenType type) {
        if (this->type() == type) {
            next();
            return;
        }
        error("");
    }
    
    Expr* id() {
        return _context.make<IDExpr>(curnext().val());
    }
    
    Expr* num() {
        auto n = std::stoul(curnext().val());
        return _context.make<NumExpr>(n);
    }
    
    Expr* hex() {
        auto n = std::stoul(curnext().val().substr(1), nullptr, 16);
        return _context.make<NumExpr>(n);
    }
    
    Expr* expr() {
        auto left = expect(&Parser::term);
        while (type() == TokenType::Plus || type() == TokenType::Minus) {
            auto op = type();
            next();
            auto right = expect(&Parser::term);
            left = _context.make<BinaryOpExpr>(op, left, right);
        }
        return left;
    }
    
    Expr* parens() {
        expect(TokenType::LeftParen);
        auto e = expect(&Parser::expr);
        expect(TokenType::RightParen);
        return e;
    }
    
    Expr* mem() {
        expect(TokenType::LeftSquareBracket);
        auto e = expect(&Parser::expr);
        expect(TokenType::RightSquareBracket);
        return _context.make<MemExpr>(e);
    }
    
    Expr* factor() {
        if (type() == TokenType::ID)
            return id();
        if (type() == TokenType::Int)
            return num();
        if (type() == TokenType::Hex)
            return hex();
        if (type() == TokenType::LeftParen)
            return expect(&Parser::parens);
        if (type() == TokenType::LeftSquareBracket)
            return expect(&Parser::mem);
        error("");
        return nullptr;
    }
    
    Expr* term() {
        auto left = expect(&Parser::factor);
        while (type() == TokenType::Mul || type() == TokenType::Div) {
            auto op = type();
            next();
            auto right = expect(&Parser::factor);
            left = _context.make<BinaryOpExpr>(op, left, right);
        }
        return left;
    }

public:
    Expr* parse() {
        return expect(&Parser::expr);
    }
};

template <typename TH>
uint64_t evalExpr(std::string const& text, TH* th) {
    auto tokens = tokenize(text);
    Parser parser(&tokens, &text);
    auto expr = parser.parse();
    return expr->eval(th);
}

template uint64_t evalExpr<PPUThread>(std::string const& text, PPUThread* th);
template uint64_t evalExpr<SPUThread>(std::string const& text, SPUThread* th);
