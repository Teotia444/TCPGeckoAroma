FROM ghcr.io/wiiu-env/devkitppc:20230218

COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20250208 /artifacts $DEVKITPRO
COPY --from=ghcr.io/wiiu-env/libnotifications:20240426 /artifacts $DEVKITPRO

WORKDIR project