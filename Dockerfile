FROM ubuntu:bionic
RUN apt-get update && apt-get install -y cmake build-essential
CMD ["cmake"]
CMD ["make package"]
