#include <opencv2/opencv.hpp>
#include <iostream>
#include <fcntl.h>      // shm_open
#include <sys/mman.h>   // mmap
#include <unistd.h>     // ftruncate, close
#include <cstring>      // memcpy

// ----- Shared Memory 설정 -----
#define SHM_NAME "/busbom_frame"
#define FRAME_WIDTH  1280
#define FRAME_HEIGHT 720
#define FRAME_CHANNELS 3
#define FRAME_SIZE (FRAME_WIDTH * FRAME_HEIGHT * FRAME_CHANNELS)

// 공유 메모리 이름
#define SHM_NAME "/busbom_frame"

int main() {
    // ----- 공유 메모리 생성 및 설정 -----
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

    // 크기 설정
    if (ftruncate(shm_fd, FRAME_SIZE) == -1) {
        perror("ftruncate");
        return 1;
    }

    // 메모리 매핑
    void* shm_ptr = mmap(0, FRAME_SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // ----- RTSP 스트림 열기 -----
    std::string rtsp_url = "rtsp://192.168.0.39:554/profile2/media.smp"; // 실제 주소로 수정하기
    cv::VideoCapture cap(rtsp_url);

    if (!cap.isOpened()) {
        std::cerr << "RTSP 연결 실패" << std::endl;
        return -1;
    }

    std::cout << "RTSP 연결 성공, 프레임 수신 중..." << std::endl;

    // ✅ MP4 영상 저장 설정 (코덱: 'mp4v', 파일명: .mp4)
    std::string savePath = "/home/iam/videos/output.mp4";
    int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v'); 
    double fps = 15.0;
    cv::VideoWriter writer(savePath, fourcc, fps, cv::Size(FRAME_WIDTH, FRAME_HEIGHT));

    if (!writer.isOpened()) {
        std::cerr << "비디오 파일 저장 실패" << std::endl;
        return -1;
    }

    // ----- 프레임 수신 및 공유 메모리에 저장 -----
    cv::Mat frame;
    while (true) {
        cap >> frame;
        if (frame.empty()) continue;

        cv::Mat resized;
        cv::resize(frame, resized, cv::Size(FRAME_WIDTH, FRAME_HEIGHT));

        // 💾 mp4 저장
        writer.write(resized);

        // 📤 공유 메모리 복사
        std::memcpy(shm_ptr, resized.data, FRAME_SIZE);

    }

    // ----- 정리 -----
    writer.release();
    munmap(shm_ptr, FRAME_SIZE);
    close(shm_fd);
    //shm_unlink(SHM_NAME); // 실제 운영 시에는 제거하지 않을 수도 있음

    return 0;
}
