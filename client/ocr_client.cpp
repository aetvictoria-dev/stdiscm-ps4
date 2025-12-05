#include "ocr_client.h"
#include <QPixmap>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QFileInfo>
#include <fstream>
#include <iostream>

ImageResult::ImageResult(const QString& imagePath, QWidget* parent)
    : QWidget(parent), imagePath_(imagePath) {

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(5);

    imageLabel_ = new QLabel(this);
    QPixmap pixmap(imagePath);
    if (!pixmap.isNull()) {
        imageLabel_->setPixmap(pixmap.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    imageLabel_->setAlignment(Qt::AlignCenter);
    imageLabel_->setStyleSheet("border: 1px solid #ccc; background-color: white;");
    imageLabel_->setMinimumSize(200, 200);

    resultLabel_ = new QLabel("In progress...", this);
    resultLabel_->setWordWrap(true);
    resultLabel_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    resultLabel_->setStyleSheet("padding: 5px; background-color: #2b2b2b; color: white; border-radius: 3px;");
    resultLabel_->setMinimumHeight(60);
    resultLabel_->setMaximumWidth(200);

    layout->addWidget(imageLabel_);
    layout->addWidget(resultLabel_);
    setLayout(layout);
}

void ImageResult::setResult(const QString& text) {
    resultLabel_->setText(text.isEmpty() ? "(no text detected)" : text);
    resultLabel_->setStyleSheet("padding: 5px; background-color: #2b2b2b; color: white; border-radius: 3px;");
}

void ImageResult::setError(const QString& error) {
    resultLabel_->setText("Error: " + error);
    resultLabel_->setStyleSheet("padding: 5px; background-color: #ff4444; color: white; border-radius: 3px;");
}

OCRWorker::OCRWorker(const QString& serverAddress,
                     const QStringList& imagePaths,
                     QObject* parent)
    : QThread(parent),
      serverAddress_(serverAddress),
      imagePaths_(imagePaths),
      stopped_(false) {
}

OCRWorker::~OCRWorker() {
    stop();
    wait();
}

void OCRWorker::stop() {
    stopped_ = true;
}

// ============================================================================
// MULTITHREADING: Worker Thread Execution
// ============================================================================
// This function runs in a separate thread to avoid blocking the UI.
// It processes all images sequentially and sends them to the server via gRPC.
void OCRWorker::run() {
    // INTERPROCESS COMMUNICATION: Create gRPC channel to server
    channel_ = grpc::CreateChannel(serverAddress_.toStdString(),
                                   grpc::InsecureChannelCredentials());
    stub_ = ocrservice::OCRService::NewStub(channel_);

    int total = imagePaths_.size();
    for (int i = 0; i < total && !stopped_; ++i) {
        QString imagePath = imagePaths_[i];

        // Read image file as binary data
        std::ifstream file(imagePath.toStdString(), std::ios::binary);
        if (!file) {
            emit errorOccurred(i, "Failed to read image file");
            emit progressUpdated(i + 1, total);
            continue;
        }

        // Load entire image into memory as byte array
        std::vector<uint8_t> imageData((std::istreambuf_iterator<char>(file)),
                                       std::istreambuf_iterator<char>());
        file.close();

        // INTERPROCESS COMMUNICATION: Prepare gRPC request with Protocol Buffers
        ocrservice::ImageRequest request;
        request.set_image_data(imageData.data(), imageData.size());  // Binary image data
        QFileInfo fileInfo(imagePath);
        request.set_image_id(fileInfo.fileName().toStdString());     // Filename for server logging

        // FAULT TOLERANCE: Set 60-second timeout to prevent hanging
        grpc::ClientContext context;
        context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(60));

        // INTERPROCESS COMMUNICATION: Send request and receive streaming responses
        std::unique_ptr<grpc::ClientReader<ocrservice::OCRResponse>> reader(
            stub_->ProcessImage(&context, request));

        // Read streaming responses from server (real-time results)
        ocrservice::OCRResponse response;
        bool received = false;
        while (reader->Read(&response)) {
            received = true;
            if (response.success()) {
                // SYNCHRONIZATION: Emit signal to update UI thread-safely
                emit resultReady(i, QString::fromStdString(response.extracted_text()));
            } else {
                emit errorOccurred(i, QString::fromStdString(response.error_message()));
            }
        }

        // FAULT TOLERANCE: Check for connection errors
        grpc::Status status = reader->Finish();
        if (!status.ok() && !received) {
            emit errorOccurred(i, QString::fromStdString("Connection error: " + status.error_message()));
        }

        // SYNCHRONIZATION: Update progress bar in UI
        emit progressUpdated(i + 1, total);
    }
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      worker_(nullptr),
      totalImages_(0),
      completedImages_(0),
      batchComplete_(true) {

    setupUI();
    setWindowTitle("Distributed OCR System");
    resize(900, 700);
}

MainWindow::~MainWindow() {
    if (worker_) {
        worker_->stop();
        worker_->wait();
        delete worker_;
    }
}

void MainWindow::setupUI() {
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    QHBoxLayout* controlLayout = new QHBoxLayout();
    QLabel* serverLabel = new QLabel("Server Address:", this);
    serverAddressEdit_ = new QLineEdit("localhost:50051", this);
    serverAddressEdit_->setMinimumWidth(200);

    uploadButton_ = new QPushButton("Upload Images", this);
    uploadButton_->setMinimumHeight(35);
    uploadButton_->setStyleSheet("QPushButton { padding: 5px 15px; font-size: 14px; }");

    controlLayout->addWidget(serverLabel);
    controlLayout->addWidget(serverAddressEdit_);
    controlLayout->addStretch();
    controlLayout->addWidget(uploadButton_);

    progressBar_ = new QProgressBar(this);
    progressBar_->setMinimum(0);
    progressBar_->setMaximum(100);
    progressBar_->setValue(0);
    progressBar_->setTextVisible(true);
    progressBar_->setFormat("%v / %m images processed");
    progressBar_->setMinimumHeight(25);

    scrollArea_ = new QScrollArea(this);
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setStyleSheet("QScrollArea { border: 1px solid #ccc; background-color: #1e1e1e; }");

    resultsContainer_ = new QWidget();
    resultsLayout_ = new QGridLayout(resultsContainer_);
    resultsLayout_->setSpacing(15);
    resultsLayout_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    resultsContainer_->setLayout(resultsLayout_);

    scrollArea_->setWidget(resultsContainer_);

    mainLayout->addLayout(controlLayout);
    mainLayout->addWidget(progressBar_);
    mainLayout->addWidget(scrollArea_);

    setCentralWidget(centralWidget);

    connect(uploadButton_, &QPushButton::clicked, this, &MainWindow::onUploadClicked);
}

void MainWindow::clearResults() {
    for (auto* result : imageResults_) {
        resultsLayout_->removeWidget(result);
        delete result;
    }
    imageResults_.clear();
}

// ============================================================================
// GUI IMPLEMENTATION: Upload Button Handler
// ============================================================================
// This function handles user clicking "Upload Images" button.
// It manages batch processing and spawns worker threads.
void MainWindow::onUploadClicked() {
    // Show file dialog to select images
    QStringList filePaths = QFileDialog::getOpenFileNames(
        this,
        "Select Images",
        "",
        "Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff)"
    );

    if (filePaths.isEmpty()) {
        return;
    }

    // SYNCHRONIZATION: Lock mutex to protect shared state
    QMutexLocker locker(&mutex_);

    // Batch management: Clear previous results if last batch is complete
    if (batchComplete_) {
        clearResults();
        totalImages_ = filePaths.size();
        completedImages_ = 0;
        batchComplete_ = false;
    } else {
        // Add to existing batch if still in progress
        totalImages_ += filePaths.size();
    }

    // Update progress bar
    progressBar_->setMaximum(totalImages_);
    progressBar_->setValue(completedImages_);

    // GUI IMPLEMENTATION: Create result widgets in 4-column grid layout
    int startIndex = imageResults_.size();
    for (int i = 0; i < filePaths.size(); ++i) {
        ImageResult* result = new ImageResult(filePaths[i], resultsContainer_);
        int row = (startIndex + i) / 4;  // Calculate grid row
        int col = (startIndex + i) % 4;  // Calculate grid column
        resultsLayout_->addWidget(result, row, col);
        imageResults_.push_back(result);
    }

    // MULTITHREADING: Stop previous worker if exists
    if (worker_) {
        worker_->stop();
        worker_->wait();
        delete worker_;
    }

    // MULTITHREADING: Create and start new worker thread
    worker_ = new OCRWorker(serverAddressEdit_->text(), filePaths, this);
    worker_->setProperty("startIndex", startIndex);

    // SYNCHRONIZATION: Connect signals from worker thread to UI slots
    connect(worker_, &OCRWorker::resultReady, this, &MainWindow::onResultReady);
    connect(worker_, &OCRWorker::errorOccurred, this, &MainWindow::onErrorOccurred);
    connect(worker_, &OCRWorker::progressUpdated, this, &MainWindow::onProgressUpdated);

    worker_->start();  // Start background thread
}

// ============================================================================
// SYNCHRONIZATION: Thread-Safe UI Update Slots
// ============================================================================
// These slots receive signals from the worker thread and update the UI.
// Qt's signal/slot mechanism ensures thread-safe cross-thread communication.

void MainWindow::onResultReady(int index, const QString& text) {
    // SYNCHRONIZATION: Lock mutex before accessing shared data
    QMutexLocker locker(&mutex_);
    int actualIndex = worker_->property("startIndex").toInt() + index;
    if (actualIndex >= 0 && actualIndex < imageResults_.size()) {
        // Update the result widget with OCR text (runs in main/UI thread)
        imageResults_[actualIndex]->setResult(text);
    }
}

void MainWindow::onErrorOccurred(int index, const QString& error) {
    // SYNCHRONIZATION: Lock mutex before accessing shared data
    QMutexLocker locker(&mutex_);
    int actualIndex = worker_->property("startIndex").toInt() + index;
    if (actualIndex >= 0 && actualIndex < imageResults_.size()) {
        // Update the result widget with error message (runs in main/UI thread)
        imageResults_[actualIndex]->setError(error);
    }
}

void MainWindow::onProgressUpdated(int current, int total) {
    // SYNCHRONIZATION: Lock mutex before modifying counters
    QMutexLocker locker(&mutex_);
    completedImages_++;
    progressBar_->setValue(completedImages_);

    // Mark batch as complete when all images are processed
    if (completedImages_ >= totalImages_) {
        batchComplete_ = true;
    }
}
