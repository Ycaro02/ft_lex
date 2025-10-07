#ifndef STRING_HANDLER_H
#define STRING_HANDLER_H

#include <stdlib.h>
#include <string.h>

#define BUFF_SIZE (1024*1024)

typedef struct String {
    char *str;
    int  pos;
    int  len;
} String;

#define INLINE static inline __attribute__((always_inline))

/**
 * @brief Get the length of the input string
 * @return Length of the input string
 */
INLINE int len(String *s) {
    return (s->len);
}

/**
 * @brief Peek at the current character in the input without consuming it
 * @return The current character
 */
INLINE char peek(String *s) {
    return (s->str[s->pos]);
}


/**
 * @brief Consume and return the next character in the input
 * @return The next character
 */
INLINE char next(String *s) {
    return (s->str[s->pos++]);
}

/**
 * @brief Check if we've reached the end of the input
 * @return 1 if at end, 0 otherwise
 */
INLINE int end(String *s) {
    return (s->str[s->pos] == '\0');
}


/* utils/split.c */
void    free_split(char **split);
char	**ft_split_trim(char const *str, char c);

/* utils/trim.c */
char	*ft_strtrim(char const *s1, char const *set);

#endif /* STRING_HANDLER_H */