#include "SD.hpp"

const int SD_Pin = 21;
bool SD_Check = false;

static File root;
static File currentFile;
static bool uploading = false;

void SD_Init() {
    if (!SD.begin(SD_Pin)) {
        Serial.println("SD Card Mount Failed");
        SD_Check = false;
        return;
    }

    if (!SD.exists("/samples")) {
        SD.mkdir("/samples");
        Serial.println("Created /samples folder");
    }

    Serial.println("SD Card Ready");
    SD_Check = true;
}

bool SD_Sample(camera_fb_t *fb, GnssData data) {
    if (!SD_Check || !fb) return false;

    char base[64];
    snprintf(base, sizeof(base), "/samples/%02d%02d%02d_%lu",
             data.hour, data.min, data.sec, millis());

    String imgPath = String(base) + ".jpg";
    File imgFile = SD.open(imgPath, FILE_WRITE);
    if (!imgFile) return false;

    imgFile.write(fb->buf, fb->len);
    imgFile.close();

    String txtPath = String(base) + ".txt";
    File txtFile = SD.open(txtPath, FILE_WRITE);
    if (!txtFile) return false;

    txtFile.printf("lat=%.6f\nlon=%.6f\nalt=%.2f\nsats=%d\n",
                   data.lat, data.lon, data.alt, data.satellites);
    txtFile.close();

    return true;
}

void SD_Upload_Begin() {
    if (!SD_Check) return;

    root = SD.open("/samples");
    if (!root || !root.isDirectory()) {
        Serial.println("Upload failed: no folder");
        return;
    }

    currentFile = root.openNextFile();
    uploading = true;

    Serial.println("Upload started");
}

bool SD_Upload_Active() {
    return uploading;
}

void SD_Upload_Task() {
    static unsigned long lastUpload = 0;

    // limit rate (important)
    if (millis() - lastUpload < 200) return;
    lastUpload = millis();

    if (!uploading) return;

    if (!currentFile) {
        root.close();
        uploading = false;
        Serial.println("Upload complete");
        return;
    }

    String filename = String(currentFile.name());
    Serial.println("Uploading: " + filename);

    currentFile.close();

    // move to next file
    currentFile = root.openNextFile();
}