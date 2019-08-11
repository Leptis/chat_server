
#FROM archlinux/base:latest

#RUN mkdir -p /home/chat_server

#COPY ./cmake-build-debug/chat_server /home/chat_server/chat_server



#CMD /home/chat_server/chat_server

FROM server_core:latest

CMD /home/chat_server/chat_server