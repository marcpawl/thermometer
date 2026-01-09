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

RUN mkdir /workspaces
RUN git clone https://github.com/marcpawl/thermometer.git /workspaces

WORKDIR /workspaces

