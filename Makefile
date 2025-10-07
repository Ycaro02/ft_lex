include rsc/mk/color.mk
include rsc/mk/source.mk

NAME            =   ft_lex
CC              =   clang

all:        $(NAME)

$(NAME): $(LIBFT) $(LIST) $(OBJ_DIR) $(OBJS)
	@$(MAKE_LIBFT)
	@$(MAKE_LIST)
	@printf "$(CYAN)Compiling ${NAME} ...$(RESET)\n"
	@$(CC) $(CFLAGS) -o $(NAME) $(OBJS) $(LIBFT) $(LIST) -lm
	@printf "$(GREEN)Compiling $(NAME) done$(RESET)\n"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@printf "$(YELLOW)Compile $<$(RESET) $(BRIGHT_BLACK)-->$(RESET) $(BRIGHT_MAGENTA)$@$(RESET)\n"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -o $@ -c $<


run:
	@./rsc/docker/run.sh

lex:
	@./rsc/run_lex.sh test.l "$(word 2,$(MAKECMDGOALS))"

test_match: $(NAME)
	@./rsc/tester/test_match.sh

bonus: clear_mandatory $(NAME)

clear_mandatory:
ifeq ($(shell [ -f ${OBJ_DIR}/main.o ] && echo 0 || echo 1), 0)
	@printf "$(RED)Clean mandatory obj $(RESET)\n"
	@rm -rf ${OBJ_DIR}
endif

clean:
ifeq ($(shell [ -d ${OBJ_DIR} ] && echo 0 || echo 1), 0)
	@$(RM) $(OBJ_DIR)
	@printf "$(RED)Clean $(OBJ_DIR) done$(RESET)\n"
	@$(RM)
endif

fclean:	 clean
	@$(RM) $(NAME)
	@printf "$(RED)Clean $(NAME)$(RESET)\n"

# @ulimit -c unlimited
leak thread debug: clean $(NAME)
	@printf	"$(CYAN)CFLAGS: $(CFLAGS)$(RESET)\n"

re: clean $(NAME)



%:
	@:
