FROM ubuntu:22.04

# Éviter les prompts interactifs
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    libsdl2-2.0-0 libsdl2-dev libsdl2-ttf-2.0-0 libsdl2-ttf-dev \
    libgl1-mesa-glx libgl1-mesa-dri mesa-utils


# Installer les dépendances
RUN apt-get update && apt-get install -y \
    build-essential \
    libsdl2-dev \
    libsdl2-ttf-dev \
    libsdl2-image-dev \
    libsdl2-mixer-dev \
    valgrind \
  	binutils \
    git \
    make \
    && apt-get clean

# Définir le dossier de travail
WORKDIR /app

# Copier tous les fichiers dans le conteneur
COPY . .

# Créer les dossiers nécessaires pour la compilation
RUN mkdir -p build/obj

# Compiler le projet
RUN make all

# Définir la commande par défaut pour lancer le jeu
CMD ["./gomoku"]
