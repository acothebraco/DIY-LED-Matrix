# SmartFix Matrix Web Flasher

This update adds a GitHub Pages based browser flasher for SmartFix Matrix.

## Files

```text
web/flash/index.html
web/flash/manifest.json
.github/workflows/webflasher.yml
```

## One-time GitHub setup

1. Open the SmartFix-Matrix repository on GitHub.
2. Go to Settings -> Pages.
3. Set Source to GitHub Actions.
4. Push this update to main.
5. Wait until the workflow `Build firmware and deploy web flasher` has completed.

The web flasher will be available at:

```text
https://acothebraco.github.io/SmartFix-Matrix/
```

## Important

The browser flasher uses the full USB binary:

```text
SmartFix-Matrix-usb.bin
```

The OTA binary is still used only for OTA updates from the device web interface:

```text
SmartFix-Matrix-ota.bin
```

