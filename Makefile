################################################################################
# Makefile                                                                     #
#                                                                              #
# Description: This file contains the make rules for Project 1.                #
#                                                                              #
# Authors: Evan Kesten <ebk46@cornell.edu>                                     #
#                                                                              #
################################################################################

default: http_server

http_server:
	@gcc http_server.c -o http_server -Wall -Werror

clean:
	@rm http_server
