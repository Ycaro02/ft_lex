#include "../include/log.h"
#include "../include/regex_tree.h"


/**
 * @brief Parse a character class like [abc] or [^abc]
 * @return The root of the character class subtree
 */
static RegexTreeNode* parse_class(String *s) {
    if (peek(s) == '[') {
        next(s); // skip '['
        int neg = 0;
        if (peek(s) == '^') {
            neg = 1;
            next(s); // skip '^'
        }

        char buffer[256] = {0};
        int idx = 0;

        if (len(s) >= 255) {
            ERR("Character class too long\n");
            return (NULL);
        }

        while (!end(s) && peek(s) != ']') {
            buffer[idx++] = next(s);
        }
        buffer[idx] = '\0';

        if (peek(s) == ']') next(s); // skip ']'

        if (neg) {
            return RegexTreeNode_create(REG_CLASS_NEG, NULL, NULL, buffer, 0);
        } else {
            return RegexTreeNode_create(REG_CLASS, NULL, NULL, buffer, 0);
        }
    }

    return (NULL);
}


/**
 * @brief Parse an atom: either a character class, a parenthesized expression, or a single character
 * @return The root of the atom subtree
 */
static RegexTreeNode* parse_atom(String *s) {
    // First try to parse a character class
    RegexTreeNode* class_node = parse_class(s);
    if (class_node) {
        return class_node;
    }
    
    // Then try to parse parentheses
    if (peek(s) == '(') {
        next(s); // skip '('
        RegexTreeNode* r = parse_regex(s);
        if (peek(s) == ')') next(s); // skip ')'
        return r;
    } else {
        char c = next(s);
        return RegexTreeNode_create(REG_CHAR, NULL, NULL, NULL, c);
    }
}


/**
 * @brief Parse repetition operators (*, +, ?)
 * @return The root of the repetition subtree
 */
static RegexTreeNode* parse_repeat(String *s) {
    RegexTreeNode* atom = parse_atom(s);
    char c = peek(s);

    if (c == '*') {
        next(s);
        return RegexTreeNode_create(REG_STAR, atom, NULL, NULL, 0);
    } else if (c == '+') {
        next(s);
        return RegexTreeNode_create(REG_PLUS, atom, NULL, NULL, 0);
    } else if (c == '?') {
        next(s);
        return RegexTreeNode_create(REG_OPTIONAL, atom, NULL, NULL, 0);
    }
    return (atom);
}

/**
 * @brief Parse concatenation (AB)
 * @return The root of the concatenation subtree
 */
static RegexTreeNode* parse_concat(String *s) {
    RegexTreeNode* left = parse_repeat(s);

    while (!end(s)) {
        char c = peek(s);
        if (c == ')' || c == '|') break; // end concatenation on ')' or '|'

        RegexTreeNode* right = parse_repeat(s);
        left = RegexTreeNode_create(REG_CONCAT, left, right, NULL, 0);
    }

    return (left);
}

/**
 * @brief Parse alternation (A|B)
 * @return The root of the alternation subtree
 */
static RegexTreeNode* parse_alt(String *s) {
    RegexTreeNode* left = parse_concat(s);

    while (peek(s) == '|') {
        next(s); // skip '|'
        RegexTreeNode* right = parse_concat(s);
        left = RegexTreeNode_create(REG_ALT, left, right, NULL, 0);
    }

    return (left);
}

/**
 * @brief Parse the full regex
 * @return The root of the regex tree
 */
RegexTreeNode* parse_regex(String *s) {
    if (end(s)) return (NULL);

    return (parse_alt(s));
}
