# Teensy Loader Javascript for Chromium Browsers

This repository hosts a minimal web page and JavaScript library for flashing Teensy firmware via WebHID and opening a Serial connection.

## Quick Start

To test this repository go to the Github pages deployment here: https://coelacant1.github.io/Teensy-Loader-Javascript/Teensy-Loader-Example.html

[![Deploy to GitHub Pages](https://github.com/coelacant1/Teensy-Loader-Javascript/actions/workflows/DeployPages.yml/badge.svg?branch=main)](https://github.com/coelacant1/Teensy-Loader-Javascript/actions/workflows/DeployPages.yml)
[![pages-build-deployment](https://github.com/coelacant1/Teensy-Loader-Javascript/actions/workflows/pages/pages-build-deployment/badge.svg?branch=gh-pages)](https://github.com/coelacant1/Teensy-Loader-Javascript/actions/workflows/pages/pages-build-deployment)

## Repository Structure

- Teensy-Loader.js  
  The core library containing classes for FirmwareFile, TeensyFlasher, and SerialPortManager.

- Teensy-Loader-Example.html  
  A simple HTML page demonstrating how to:
  - Load a local .hex or .bin file
  - Select a Teensy device via WebHID
  - Flash the firmware
  - Open/close a serial port to display text output

- firmware/
  Example firmware files (e.g., blink_slow_Teensy40.hex, blink_slow_Teensy41.hex). These can be downloaded directly via links or used with the test page.

- .github/workflows/DeployPages.yml  
  A GitHub Actions workflow file that builds and deploys this project to the gh-pages branch, allowing GitHub Pages hosting.

## Local Development

1. Clone or download this repository.
2. If you want to run locally, open a terminal in the project’s directory and start a local server. For example:
```bash
python -m http.server
```
3. Visit http://localhost:8000/Teensy-Loader-Example.html in your browser (Chrome or Edge) and navigate to the HTML file.
4. Select your firmware file and click “Select Teensy Device” to pick a board via WebHID, then “Upload Firmware” to flash. You can also open or close a serial port to see debug output from the Teensy.

## Questions and Support

For additional information or recommendations, use the **Discussions** tab on GitHub.

## Contributing

Contributions are welcome! To contribute:
1. Fork the repository on GitHub.
2. Commit your changes with a descriptive message (git commit -m 'Add YourFeature').
3. Push the branch (git push origin main).
4. Submit a pull request on GitHub.

## License Agreement

This project is licensed under the [AGPL-3.0](https://choosealicense.com/licenses/agpl-3.0/). This ensures modifications and contributions benefit the community.
