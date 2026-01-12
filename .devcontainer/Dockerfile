FROM espressif/idf:release-v5.5

# Install additional dependencies if needed
RUN apt-get update && apt-get install -y \
    curl \
    gh \
    git \
    libfontconfig1 \
    libxi6 \
    libxtst6 \
    libxrender1 \
    procps \
    psmisc \
    python3 \
    python3-pip \
    python3-venv \
    tar \
    vim \
    usbutils

# Ensure the user has access to serial ports
RUN usermod -aG dialout root

COPY idf_cmake.sh /opt/esp/idf_cmake.sh
RUN chmod +x /opt/esp/idf_cmake.sh
RUN echo ". /opt/esp/idf/export.sh" >> /root/.bashrc

COPY clion.2025.1.7.tar.gz /tmp/clion.tar.gz
RUN mkdir -p /root/.cache/JetBrains/RemoteDev/dist
RUN tar -C /root/.cache/JetBrains/RemoteDev/dist -xzf /tmp/clion.tar.gz
RUN rm /tmp/clion.tar.gz
ENV PATH=${PATH}:/root/.cache/JetBrains/RemoteDev/dist/clion-2025.1.7/bin

#RUN git clone https://github.com/marcpawl/thermometer.git /workspaces

WORKDIR /workspaces

CMD sleep 360000
