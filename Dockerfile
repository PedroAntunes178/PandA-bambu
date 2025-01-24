FROM ubuntu:20.04

# Set non-interactive mode for apt
ENV DEBIAN_FRONTEND=noninteractive

# Enable multilib support and install dependencies
RUN dpkg --add-architecture i386 && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
        software-properties-common \
        build-essential \
        ca-certificates \
        gcc-multilib \
        git \
        iverilog \
        verilator \
        wget && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# Add Git PPA for the latest Git version
RUN echo "deb http://ppa.launchpad.net/git-core/ppa/ubuntu $(. /etc/os-release && echo $UBUNTU_CODENAME) main" >> /etc/apt/sources.list.d/git-core.list && \
    apt-key adv --keyserver keyserver.ubuntu.com --recv-keys A1715D88E1DF1F24 && \
    apt-get update && apt-get install -y git && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# Download and configure the Bambu AppImage
RUN wget https://release.bambuhls.eu/appimage/bambu-latest.AppImage && \
    chmod +x bambu-*.AppImage && \
    ln -sf /bambu-latest.AppImage /usr/bin/bambu && \
    ln -sf /bambu-latest.AppImage /usr/bin/spider && \
    ln -sf /bambu-latest.AppImage /usr/bin/tree-panda-gcc && \
    ln -sf /bambu-latest.AppImage /usr/bin/clang-12

# Clone and configure the PandA-bambu repository
RUN git clone --depth 1 --filter=blob:none --sparse https://github.com/ferrandi/PandA-bambu.git && \
    cd PandA-bambu && \
    git sparse-checkout set documentation/tutorial_date_2022 && \
    cd .. && \
    mv PandA-bambu/documentation/tutorial_date_2022/ /bambu-tutorial

# Set the working directory
WORKDIR /workspace

# Default command (optional)
CMD ["bash"]

