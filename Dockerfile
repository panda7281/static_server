FROM ubuntu:22.04

COPY ./build/StaticServer /app/StaticServer

WORKDIR /app

EXPOSE 8080

VOLUME [ "/app/www", "/app/config" ]

CMD [ "./StaticServer" ]
