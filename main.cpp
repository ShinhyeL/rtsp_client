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


    // ✅ 여기서 저장 파일 경로를 지정
    std::string filename = "/home/iam/videos/output.avi";

    // ✅ VideoWriter 객체 생성
    cv::VideoWriter writer(filename,
                           cv::VideoWriter::fourcc('M','J','P','G'),
                           30,
                           cv::Size(FRAME_WIDTH, FRAME_HEIGHT));

    if (!writer.isOpened()) {
        std::cerr << "VideoWriter 초기화 실패" << std::endl;
        return -1;
    }

    // ----- 프레임 수신 및 공유 메모리에 저장 -----
    cv::Mat frame;
    while (true) {
        cap >> frame;
        if (frame.empty()) continue;

        // 카메라 사이즈와 동일하게 수정
        std::memcpy(shm_ptr, frame.data, FRAME_SIZE);

        writer.write(frame);  // 영상 파일에 프레임 기록

        // 디버깅용 출력
        cv::imwrite("rtsp.jpg", frame);
        if (cv::waitKey(1) == 27) break; // ESC 키 누르면 종료
    }

    // ----- 정리 -----
    munmap(shm_ptr, FRAME_SIZE);
    close(shm_fd);
    //shm_unlink(SHM_NAME); // 실제 운영 시에는 제거하지 않을 수도 있음

    return 0;
}
