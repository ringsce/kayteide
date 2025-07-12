// Jenkinsfile for KayteIDE

pipeline {
    agent {
        // Use a label to target specific Jenkins agents (nodes)
        // For macOS builds, you'll need an agent with Xcode and Qt installed.
        // For Linux builds, an agent with g++ and Qt installed.
        // You might have separate agents for different OS, or a single agent
        // with both capabilities if using VMs/containers.
        label 'macos' // Example: Replace with your actual agent label
        // OR agent { label 'linux-agent' }
        // OR agent any // If a single agent can handle both
    }

    options {
        // Clean workspace before starting the build
        // This is good practice for consistent builds but can be slow for large projects.
        // skipDefaultCheckout true // If you handle SCM checkout explicitly
        buildDiscarder(logRotator(numToKeepStr: '10')) // Keep last 10 builds
        timestamps() // Add timestamps to console output
    }

    environment {
        // Set up environment variables for Qt and CMake
        // Adjust these paths based on your Jenkins agent's Qt installation.
        // For macOS, often from Homebrew or official installer.
        // For Linux, typically from official installer or package manager.

        // Example for macOS (adjust path to your Qt installation)
        QT_PATH = '/usr/local/opt/qt6' // Homebrew Qt6 path example
        // Or if installed by official Qt installer: '/Users/jenkins/Qt/6.x.x/macos'

        // Example for Linux (adjust path to your Qt installation)
        // QT_PATH = '/opt/Qt/6.x.x/gcc_64'

        // CMake prefix path to find Qt6 CMake modules
        // This is crucial for CMake to locate Qt6 components.
        CMAKE_PREFIX_PATH = "${QT_PATH}/lib/cmake"

        // Build directory name
        BUILD_DIR = 'build'

        // Unique identifier for macOS bundle (from Info.plist)
        MACOS_BUNDLE_IDENTIFIER = 'com.kd.kayteide'
    }

    stages {
        stage('Checkout') {
            steps {
                // Checkout your source code from SCM (e.g., Git)
                // If using Jenkins declarative pipeline's default checkout, you can remove this.
                checkout scm
            }
        }

        stage('Clean Build Directory') {
            steps {
                script {
                    sh "rm -rf ${BUILD_DIR}" // Clean previous build artifacts
                }
            }
        }

        stage('Configure CMake') {
            steps {
                script {
                    // Create the build directory if it doesn't exist
                    sh "mkdir -p ${BUILD_DIR}"
                    dir("${BUILD_DIR}") {
                        // Run CMake to configure the project
                        // -DCMAKE_INSTALL_PREFIX is important for later installation/packaging
                        def cmake_config_cmd = "cmake .. -DCMAKE_INSTALL_PREFIX=${WORKSPACE}/install -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"

                        // macOS specific: if building universal, ensure archs are set
                        if (isUnix() && isMac()) {
                            // CMAKE_OSX_ARCHITECTURES is typically set in the CMakeLists.txt itself as a default
                            // but can be overridden here if needed for specific build scenarios.
                            // For example: cmake_config_cmd += " -DCMAKE_OSX_ARCHITECTURES=\"arm64;x86_64\""
                        }
                        sh cmake_config_cmd
                    }
                }
            }
        }

        stage('Build KayteIDE') {
            steps {
                script {
                    dir("${BUILD_DIR}") {
                        // Build the project using CMake --build
                        // -j auto uses as many jobs as CPU cores
                        sh "cmake --build . -j auto"
                    }
                }
            }
        }

        stage('Run Tests') {
            // This stage is a placeholder. You'd add commands to run your tests here.
            // Example for CTest (if you have tests defined in CMake):
            // sh "cd ${BUILD_DIR} && ctest --output-on-failure"
            steps {
                echo 'Skipping tests for now. Implement your test commands here.'
            }
        }

        stage('Package macOS App') {
            // This stage only runs on macOS agents
            when {
                expression { isUnix() && isMac() }
            }
            steps {
                script {
                    // Define the path to the built .app bundle
                    def app_bundle = "${BUILD_DIR}/KayteIDE.app"

                    // Define the path to macdeployqt
                    // This path will vary based on your Qt installation on the macOS agent
                    def macdeployqt_path = "${QT_PATH}/bin/macdeployqt"

                    // Check if macdeployqt exists
                    sh "test -f ${macdeployqt_path}"

                    // Run macdeployqt to bundle Qt frameworks and plugins
                    // --qmldir is useful if you have QML files to ensure QML plugins are included
                    sh "${macdeployqt_path} ${app_bundle} -dmg"
                    // The -dmg option tells macdeployqt to also create a .dmg installer
                    // You might need to add --no-strip if you intend to debug the bundled app.
                    // For a distributable app, stripping debug symbols is good.
                }
            }
            post {
                success {
                    // Archive the generated .app bundle and .dmg for artifacts
                    archiveArtifacts artifacts: "${BUILD_DIR}/KayteIDE.app", fingerprint: true
                    archiveArtifacts artifacts: "${BUILD_DIR}/KayteIDE.dmg", fingerprint: true
                }
            }
        }

        stage('Package Linux App') {
            // This stage only runs on Linux agents
            when {
                expression { isUnix() && !isMac() }
            }
            steps {
                script {
                    // For Linux, typically you'd use linuxdeployqt or fpm to create a .AppImage, .deb, or .rpm
                    // Or simply create a tarball of the built executable and its dependencies.

                    // Example: Basic tarball of the executable and runtime Qt libs
                    def app_executable = "${BUILD_DIR}/bin/KayteIDE" // Path to your executable within the build dir
                    def install_dir = "${WORKSPACE}/install/KayteIDE"

                    // Create an install directory
                    sh "mkdir -p ${install_dir}/bin"
                    sh "cp ${app_executable} ${install_dir}/bin/"

                    // Use windeployqt (for Windows), macdeployqt (for macOS)
                    // For Linux, you often manually copy libs or use linuxdeployqt.
                    // Basic copy of necessary Qt libs (might need more depending on your app's complexity)
                    // This is a simplified example; full dependency collection is more complex.
                    // A better approach is often using `ldd` to find dynamic libs and copy them,
                    // or use `linuxdeployqt`.
                    sh "cp -L ${QT_PATH}/lib/libQt6Core.so* ${install_dir}/bin/"
                    sh "cp -L ${QT_PATH}/lib/libQt6Gui.so* ${install_dir}/bin/"
                    sh "cp -L ${QT_PATH}/lib/libQt6Widgets.so* ${install_dir}/bin/"
                    // ... copy other needed libs and plugins (platforms, imageformats, etc.)

                    // Create a tarball
                    sh "tar -czvf KayteIDE-linux.tar.gz -C ${WORKSPACE}/install KayteIDE"
                }
            }
            post {
                success {
                    // Archive the generated tarball for artifacts
                    archiveArtifacts artifacts: "KayteIDE-linux.tar.gz", fingerprint: true
                }
            }
        }

        stage('Deploy') {
            // This stage is a placeholder for deployment logic.
            // This could involve uploading artifacts to a release server,
            // deploying to an S3 bucket, etc.
            steps {
                echo 'Deployment stage: Replace with your deployment script.'
                // Example: Upload to an S3 bucket
                // withAWS(credentials: 'aws-credentials') {
                //     sh "aws s3 cp ${BUILD_DIR}/KayteIDE.dmg s3://your-release-bucket/kayteide/${env.BUILD_NUMBER}/KayteIDE.dmg"
                // }
            }
        }
    }

    post {
        always {
            // Clean up workspace to free disk space (optional, but good for large projects)
            // cleanWs()
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

// Helper functions for conditional stages (optional, but improves readability)
def isMac() {
    return System.getProperty('os.name').toLowerCase().contains('mac')
}

def isUnix() {
    def osName = System.getProperty('os.name').toLowerCase()
    return osName.contains('mac') || osName.contains('linux') || osName.contains('unix')
}