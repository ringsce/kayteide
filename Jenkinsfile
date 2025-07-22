// Jenkinsfile for KayteIDE

pipeline {
    agent {
        // Use a label to target specific Jenkins agents (nodes)
        // We'll use 'any' and then 'when' conditions for OS-specific stages
        agent any
    }

    options {
        // Clean workspace before starting the build
        buildDiscarder(logRotator(numToKeepStr: '10')) // Keep last 10 builds
        timestamps() // Add timestamps to console output
        // Add a quiet period to allow SCM checkouts to finish before building
        // This can prevent race conditions if SCM triggers are very frequent.
        quietPeriod(0)
    }

    environment {
        // Define common environment variables
        // BUILD_DIR is now global
        BUILD_DIR = 'build'

        // Dynamic RELEASE_CYCLE based on current month and year
        // This will be used for artifact naming and deployment paths.
        // Format: YYYYMM (e.g., 202507 for July 2025)
        RELEASE_CYCLE = sh(returnStdout: true, script: 'date +%Y%m').trim()

        // macOS specific environment variables (will be set conditionally if agent is macOS)
        QT_PATH_MACOS = '/usr/local/opt/qt6' // Homebrew Qt6 path example
        MACOS_BUNDLE_IDENTIFIER = 'com.kd.kayteide'

        // Linux specific environment variables (will be set conditionally if agent is Linux)
        QT_PATH_LINUX = '/opt/Qt/6.x.x/gcc_64' // Example for Linux Qt installation
    }

    stages {
        stage('Checkout') {
            steps {
                checkout scm
            }
        }

        stage('Clean Build Directory') {
            steps {
                sh "rm -rf ${BUILD_DIR}"
            }
        }

        stage('Configure CMake') {
            steps {
                script {
                    sh "mkdir -p ${BUILD_DIR}"
                    dir("${BUILD_DIR}") {
                        def cmake_prefix_path
                        def qt_path_env

                        // Determine QT_PATH and CMAKE_PREFIX_PATH based on OS
                        if (isMac()) {
                            qt_path_env = env.QT_PATH_MACOS
                            cmake_prefix_path = "${qt_path_env}/lib/cmake"
                        } else if (isLinux()) {
                            qt_path_env = env.QT_PATH_LINUX
                            cmake_prefix_path = "${qt_path_env}/lib/cmake"
                        } else {
                            error("Unsupported operating system for build agent.")
                        }

                        // Pass the determined QT_PATH to CMake if your CMakeLists.txt needs it
                        // You can also simply rely on CMAKE_PREFIX_PATH if CMake finds Qt correctly.
                        def cmake_config_cmd = "cmake .. -DCMAKE_INSTALL_PREFIX=${WORKSPACE}/install -DCMAKE_PREFIX_PATH=${cmake_prefix_path}"

                        sh cmake_config_cmd
                    }
                }
            }
        }

        stage('Build KayteIDE') {
            steps {
                script {
                    dir("${BUILD_DIR}") {
                        sh "cmake --build . -j auto"
                    }
                }
            }
        }

        stage('Run Tests') {
            steps {
                echo 'Skipping tests for now. Implement your test commands here.'
            }
        }

        stage('Package macOS App') {
            when {
                expression { isMac() } // Use helper function
            }
            steps {
                script {
                    def app_bundle = "${BUILD_DIR}/KayteIDE.app"
                    def macdeployqt_path = "${env.QT_PATH_MACOS}/bin/macdeployqt" // Use macOS specific path

                    sh "test -f ${macdeployqt_path}" // Verify macdeployqt existence
                    sh "${macdeployqt_path} ${app_bundle} -dmg"
                }
            }
            post {
                success {
                    // Archive artifacts with RELEASE_CYCLE in their name for clarity
                    archiveArtifacts artifacts: "${BUILD_DIR}/KayteIDE.app", fingerprint: true
                    archiveArtifacts artifacts: "${BUILD_DIR}/KayteIDE.dmg", fingerprint: true
                    // You might want to rename the DMG to include the RELEASE_CYCLE and Build Number
                    sh "mv ${BUILD_DIR}/KayteIDE.dmg ${BUILD_DIR}/KayteIDE-macOS-${env.RELEASE_CYCLE}-${env.BUILD_NUMBER}.dmg"
                    archiveArtifacts artifacts: "${BUILD_DIR}/KayteIDE-macOS-${env.RELEASE_CYCLE}-${env.BUILD_NUMBER}.dmg", fingerprint: true
                }
            }
        }

        stage('Package Linux App') {
            when {
                expression { isLinux() } // Use helper function
            }
            steps {
                script {
                    def app_executable = "${BUILD_DIR}/bin/KayteIDE"
                    def install_dir = "${WORKSPACE}/install/KayteIDE"

                    sh "mkdir -p ${install_dir}/bin"
                    sh "cp ${app_executable} ${install_dir}/bin/"

                    // For Linux, consider using linuxdeployqt for proper dependency bundling
                    // Or implement a more robust manual copy of Qt libraries and plugins.
                    // Example (simplified):
                    // You'd need to correctly identify and copy all necessary Qt libraries (e.g., with `ldd` and `readelf`)
                    // and relevant Qt plugins (platforms, imageformats, etc.) from ${env.QT_PATH_LINUX}.
                    // A proper way would be to create a deploy script that runs `linuxdeployqt`.
                    // For example: sh "${env.QT_PATH_LINUX}/bin/linuxdeployqt ${app_executable} -appimage"
                    // If you go this route, ensure linuxdeployqt is installed on your Linux agent.

                    // For this example, keep the basic tarball (highly simplified for full app packaging)
                    sh "tar -czvf KayteIDE-linux-${env.RELEASE_CYCLE}-${env.BUILD_NUMBER}.tar.gz -C ${WORKSPACE}/install KayteIDE"
                }
            }
            post {
                success {
                    // Archive the generated tarball
                    archiveArtifacts artifacts: "KayteIDE-linux-${env.RELEASE_CYCLE}-${env.BUILD_NUMBER}.tar.gz", fingerprint: true
                }
            }
        }

        stage('Deploy') {
            steps {
                echo "Deployment stage: Uploading artifacts for release cycle ${env.RELEASE_CYCLE}"
                script {
                    // Example: Upload artifacts to a structured release folder
                    // Ensure your S3 credentials (or other deployment method) are set up in Jenkins.
                    // This example assumes you have an 'aws-credentials' ID configured in Jenkins.

                    // Check if on macOS and upload .dmg
                    if (isMac()) {
                        withAWS(credentials: 'aws-credentials') {
                            sh "aws s3 cp ${BUILD_DIR}/KayteIDE-macOS-${env.RELEASE_CYCLE}-${env.BUILD_NUMBER}.dmg s3://your-release-bucket/kayteide/${env.RELEASE_CYCLE}/KayteIDE-macOS-${env.RELEASE_CYCLE}-${env.BUILD_NUMBER}.dmg"
                        }
                    }
                    // Check if on Linux and upload .tar.gz
                    else if (isLinux()) {
                        // Assuming your Linux agent also has AWS CLI and credentials configured
                        withAWS(credentials: 'aws-credentials') {
                            sh "aws s3 cp KayteIDE-linux-${env.RELEASE_CYCLE}-${env.BUILD_NUMBER}.tar.gz s3://your-release-bucket/kayteide/${env.RELEASE_CYCLE}/KayteIDE-linux-${env.RELEASE_CYCLE}-${env.BUILD_NUMBER}.tar.gz"
                        }
                    }
                }
            }
        }
    }

    post {
        always {
            // Clean up workspace to free disk space (optional, but good for large projects)
            cleanWs()
        }
        success {
            echo 'KayteIDE build successful! 🎉'
        }
        failure {
            echo 'KayteIDE build failed! 😢'
            // Send notifications, e.g., to Slack or email
            // mail to: 'your-email@example.com', subject: "KayteIDE Build Failed: ${env.JOB_NAME} #${env.BUILD_NUMBER}", body: "${env.BUILD_URL}"
        }
    }
}

// Helper functions for conditional stages
def isMac() {
    return env.AGENT_OS == 'macOS' // Jenkins populates AGENT_OS for declarative pipelines
}

def isLinux() {
    return env.AGENT_OS == 'Linux'
}

// Note: isUnix() is often not needed if you distinguish Mac/Linux clearly.
// System.getProperty('os.name') is for script{} blocks.
// For `when` conditions, `AGENT_OS` environment variable (set by Jenkins) is more reliable.