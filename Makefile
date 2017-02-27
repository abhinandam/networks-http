################################################################################
# Makefile                                                                     #
#                                                                              #
# Description: This file contains the make rules for Project 1.                #
#                                                                              #
# Authors: Evan Kesten <ebk46@cornell.edu>                                     #
#                                                                              #
################################################################################

default: lisod

lisod:
	@gcc -o lisod request_handler.c responses.h http_server.c -Wall -Werror

clean:
	@rm lisod
