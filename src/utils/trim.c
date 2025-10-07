#include "../../include/string_handler.h"

static int	is_char_in_set(char c, char const *set) {
	int	i = 0;

	while (set[i]) {
		if (set[i] == c) {
			return (1);
        }
		i++;
	}
	return (0);
}

char	*ft_strtrim(char const *s1, char const *set) {
	char	*str = NULL;
	size_t	size = 0;
	int		i = 0;

	if (s1 == NULL || set == NULL) {
		return (NULL);
    }
	size = strlen(s1);
	while (is_char_in_set(s1[i], set) == 1) {
		i++;
    }
	if (s1[i] == '\0') {
		return (strdup(""));
    }
	while (is_char_in_set(s1[size - 1], set) == 1) {
		size--;
    }
	str = calloc(sizeof(char), (size - i) + 1);
	if (str == NULL) {
		return (NULL);
    }
	while (size != (size_t)i && (int)(size - i - 1) >= 0) {
		str[size - i - 1] = s1[size - 1];
		size--;
	}
	return (str);
}
