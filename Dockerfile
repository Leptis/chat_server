
#FROM archlinux/base:latest

#RUN mkdir -p /home/chat_server

# FROM leptis/server_core:latest
#
# COPY ./cmake-build-debug/chat_server /home/chat_server/chat_server



#CMD /home/chat_server/chat_server

FROM leptis/server_core:latest

EXPOSE 9000

CMD /home/chat_server/chat_server