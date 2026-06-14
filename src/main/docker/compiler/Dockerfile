# @see https://hub.docker.com/_/ubuntu
FROM ubuntu:24.04 AS runner

# Installs dependencies.
RUN apt-get update --yes \
	&& apt-get install --yes \
		bison \
		cmake \
		flex \
		gcc \
		g++ \
		git \
		make \
		libglfw3-dev \
		libx11-dev \
		libxrandr-dev \
		libxi-dev \
		libxinerama-dev \
		libxcursor-dev \
		libgl1-mesa-dev \
		libasound2-dev \
		xvfb \
	&& rm --force --recursive /var/lib/apt/lists/*

# Pre-clones raylib so FetchContent doesn't need network access at cmake time.
RUN git clone --depth 1 --branch 5.5 https://github.com/raysan5/raylib.git /opt/raylib
