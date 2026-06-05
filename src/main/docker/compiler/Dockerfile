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
	&& rm --force --recursive /var/lib/apt/lists/*
