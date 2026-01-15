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
    openssh-server \
    tar \
    vim \
    usbutils

# Create necessary directories
RUN mkdir -p /run/sshd && \
    chmod 0755 /run/sshd

# Configure SSH
RUN ssh-keygen -A && \
    sed -i 's/^#PermitRootLogin.*/PermitRootLogin yes/' /etc/ssh/sshd_config && \
    sed -i 's/^#PasswordAuthentication.*/PasswordAuthentication yes/' /etc/ssh/sshd_config && \
    sed -i 's/^#UsePrivilegeSeparation.*/UsePrivilegeSeparation no/' /etc/ssh/sshd_config && \
    echo 'root:changeme' | chpasswd

EXPOSE 22

# Ensure the user has access to serial ports
RUN usermod -aG dialout root

COPY idf_cmake.sh /opt/esp/idf_cmake.sh
RUN chmod +x /opt/esp/idf_cmake.sh

RUN echo "echo .bashrc" >> /root/.bashrc
RUN echo "env | grep IDF" >> /root/.bashrc
RUN echo "source /opt/esp/idf/export.sh" >> /root/.bashrc
RUN echo "echo .profile" >> /root/.profile
RUN echo "env" >> /root/.profile
#RUN echo "export IDF_GITHUB_ASSETS=" >> /root/.profile
#RUN echo "export IDF_CCACHE_ENABLE=1" >> /root/.profile
RUN echo "export IDF_PATH=/opt/esp/idf" >> /root/.profile
#RUN echo "export IDF_PYTHON_CHECK_CONSTRAINTS=no" >> /root/.profile
RUN echo "export IDF_TOOLS_PATH=/opt/esp" >> /root/.profile
RUN echo "source /opt/esp/idf/export.sh" >> /root/.profile

CMD ["/usr/sbin/sshd", "-D"]
