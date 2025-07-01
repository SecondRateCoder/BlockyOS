FROM ubuntu:latest

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    qemu-system-x86 \
    qemu-utils \
    x11vnc \
    xvfb && \
    rm -rf /var/lib/apt/lists/*

# Install dependencies for your project
# ...

EXPOSE 6080
CMD ["/usr/bin/Xvfb", ":1", "-screen", "0", "1024x768x24", "&"]