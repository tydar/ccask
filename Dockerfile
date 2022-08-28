### Stage 1
FROM debian:bullseye AS build
RUN apt update && apt upgrade -y
RUN apt install -y make gcc

WORKDIR /app
COPY . .
RUN make

### Stage 2
FROM gcr.io/distroless/cc-debian11
COPY --from=build /app/build/ccask /app/build/ccask
WORKDIR /app
CMD ["/app/build/ccask"]
