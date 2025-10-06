CFLAGS			=	-Wall -Wextra -Werror -O3

OBJ_DIR			=	obj

ALL_SRC_DIR 	=	obj

SRC_DIR 		=	src

MAIN_MANDATORY 	=	main.c

SRCS			=	log.c

OBJS 			= $(addprefix $(OBJ_DIR)/, $(SRCS:.c=.o))

RM			=	rm -rf

ifeq ($(findstring bonus, $(MAKECMDGOALS)), bonus)
ASCII_NAME	= "bonus"
SRCS += $(SRCS_BONUS)
else
SRCS += $(MAIN_MANDATORY)
endif

ifeq ($(findstring leak, $(MAKECMDGOALS)), leak)
CFLAGS = -Wall -Wextra -Werror -g3 -fsanitize=address
else ifeq ($(findstring thread, $(MAKECMDGOALS)), thread)
CFLAGS = -Wall -Wextra -Werror -g3 -fsanitize=thread
else ifeq ($(findstring debug, $(MAKECMDGOALS)), debug)
CFLAGS = -Wall -Wextra -Werror -g3
endif
