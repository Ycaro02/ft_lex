#include "../../include/string_handler.h"

static int	count_word(char const *s, char c) {
	int	i = 0;
	int	count = 0;

	while (s[i]) {
		while (s[i] == c) {
			i++;
        }
		if (s[i] != c && s[i]) {
			count++;
        }
		while (s[i] != c && s[i]){
            i++;
        }	
	}
	return (count);
}

static int	is_in_set(char c, char set) {
    return (c == set);
}

static char	**ft_make_split(char **dest, char const *str, char c, int j) {
	size_t	len = 0;
	int		i = 0;

	while (str[i]) {
		if (is_in_set(str[i], c) == 1) {
			i++;
		} else {
			len++;
			i++;
			if (str[i] == c || (str[i] == '\0' && j < count_word(str, c))) {
				dest[j] = calloc(sizeof(char), len + 1);
				strncpy(dest[j], &str[i - len], len + 1);
				len = 0;
				char *trim_path = ft_strtrim(dest[j], "\b\t\n\v\f\r ");
				free(dest[j]);
				dest[j] = trim_path;
				j++;
			}
		}
	}
	dest[j] = NULL;
	return (dest);
}

char	**ft_split_trim(char const *str, char c) {
	char	**dest = NULL;
	int		j = 0;
	int		nb_word = 0;

	if (str == NULL) {
		return (NULL);
	}
	nb_word = count_word(str, c);
	dest = malloc(sizeof(char *) * (nb_word + 1));
	if (dest == NULL) {
		return (NULL);
	}
	dest = ft_make_split(dest, str, c, j);
	return (dest);
}

void   free_split(char **split) {
    int i = 0;

    if (!split) return;
    while (split[i]) {
        free(split[i]);
        i++;
    }
    free(split);
}