# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: aykrifa <aykrifa@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/11/17 12:32:52 by aykrifa           #+#    #+#              #
#                                                                              #
# **************************************************************************** #

# directories
SRC_DIR   = sources
CLASS_DIR = classes
OBJ_DIR   = build

# Sources files listing
SRC_FILES = \
	main.cpp \
	\

# Classes files listing
CLASS_FILES = \
	BatchQueue.cpp \

CLASSES = $(addprefix $(CLASS_DIR)/, $(CLASS_FILES))
SOURCES = $(addprefix $(SRC_DIR)/,$(SRC_FILES))

# include directories
INC_DIR = \
	includes \
		$(CLASS_DIR)/ \

##################################################################################
#compilation
NAME		= checker
CC			= c++
CFLAGS 		= -Wall -Wextra -Werror -std=c++17 -MMD -MP -g3
PREPROC		=
INCLUDES	= $(addprefix -I, $(INC_DIR))

# Objets (même structure que SOURCES ou CLASSES mais dans objs/)
OBJS = $(CLASSES:%.cpp=$(OBJ_DIR)/%.o) \
		$(SOURCES:%.cpp=$(OBJ_DIR)/%.o)

DEPS = $(OBJS:.o=.d)

all: $(NAME)
.PHONY: all

# LINK
$(NAME): $(OBJS)
	$(CC) $(OBJS) \
		-o $@

#compilation
$(OBJ_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(INCLUDES) \
		$< -o $@

# COMMENT
-include $(DEPS)

clean:
	rm -rf $(OBJ_DIR)
.PHONY: clean

fclean: clean
	rm -f $(NAME)
.PHONY: fclean

re: fclean
	make all
.PHONY: re

bear:
	bear -- make re
.PHONY: bear
