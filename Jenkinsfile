// Jenkinsfile for KayteIDE
//
// Runs on either a macOS or a Linux Jenkins agent (detected at runtime via
// `uname`, not a hardcoded label or a manually-set env var):
//   - On macOS: builds a single universal (arm64 + x86_64) KayteIDE.app,
//     targeting macOS 12+ (see MACOS_DEPLOYMENT_TARGET below) via an
//     official universal Qt6 (see ci/mac/ensure-universal-qt.sh). AND,
//     in parallel, builds three Linux artifacts inside small VMs:
//       - Alpine Linux, arm64 + amd64, managed by hand in UTM (see
//         ci/vm/README.md for the one-time VM setup this depends on).
//       - Debian 12 (bookworm), arm64, managed by `vagrant` + the
//         `vagrant-qemu` provider instead of UTM - besides Alpine, so the
//         build is also verified against a glibc-based distro (see
//         ci/vagrant/README.md for the one-time box setup this depends on).
//   - On Linux: builds KayteIDE natively for whatever architecture the
//     agent itself is (arm64 or amd64) - no VM needed there.

pipeline {
    agent any

    options {
        buildDiscarder(logRotator(numToKeepStr: '10'))
        timestamps()
        quietPeriod(0)
        // The Alpine and Debian build VMs are a shared resource - don't let
        // two builds fight over starting/stopping them at once.
        disableConcurrentBuilds()
    }

    environment {
        BUILD_DIR = 'build'
        RELEASE_CYCLE = sh(returnStdout: true, script: 'date +%Y%m').trim()

        // macOS universal build. Qt's official "clang_64" package is a
        // universal2 (arm64 + x86_64) build since Qt 6.x - Homebrew's Qt6
        // is not, so it's fetched via aqtinstall instead of Homebrew.
        // Bump QT_VERSION as needed; check availability with
        // `python3 -m aqt list-qt mac desktop`.
        QT_VERSION = '6.7.3'
        QT_CACHE_DIR = "${HOME}/ci-cache/qt"
        MACOS_ARCHITECTURES = 'arm64;x86_64'
        MACOS_BUNDLE_IDENTIFIER = 'com.kd.kayteide'
        // Minimum macOS version the universal build supports at runtime.
        MACOS_DEPLOYMENT_TARGET = '12.0'

        // Linux native build (when the agent itself is Linux)
        QT_PATH_LINUX = '/opt/Qt/6.x.x/gcc_64'
    }

    stages {
        stage('Checkout') {
            steps {
                checkout scm
            }
        }

        stage('Detect agent OS/arch') {
            steps {
                script {
                    env.NODE_OS = sh(returnStdout: true, script: 'uname').trim()
                    def rawArch = sh(returnStdout: true, script: 'uname -m').trim()
                    env.NODE_ARCH = (rawArch == 'aarch64' || rawArch == 'arm64') ? 'arm64'
                        : (rawArch == 'x86_64' ? 'amd64' : rawArch)
                    echo "Building on ${env.NODE_OS}/${env.NODE_ARCH}"
                }
            }
        }

        stage('Clean workspace') {
            steps {
                sh "rm -rf ${BUILD_DIR} linux-vm-out-alpine-arm64 linux-vm-out-alpine-amd64 linux-vm-out-debian12-arm64"
            }
        }

        stage('macOS: Fetch universal Qt6') {
            when { expression { env.NODE_OS == 'Darwin' } }
            steps {
                script {
                    env.QT_PATH_MACOS = sh(
                        returnStdout: true,
                        script: "ci/mac/ensure-universal-qt.sh '${QT_VERSION}' '${QT_CACHE_DIR}'"
                    ).trim()
                }
            }
        }

        stage('macOS: Configure & Build (universal)') {
            when { expression { env.NODE_OS == 'Darwin' } }
            steps {
                sh """
                    mkdir -p ${BUILD_DIR}
                    cmake -S . -B ${BUILD_DIR} \
                        -DCMAKE_OSX_ARCHITECTURES=${MACOS_ARCHITECTURES} \
                        -DCMAKE_OSX_DEPLOYMENT_TARGET=${MACOS_DEPLOYMENT_TARGET} \
                        -DCMAKE_INSTALL_PREFIX=${WORKSPACE}/install \
                        -DCMAKE_PREFIX_PATH=${QT_PATH_MACOS}/lib/cmake
                    cmake --build ${BUILD_DIR} -j "\$(sysctl -n hw.ncpu)"
                """
            }
        }

        stage('macOS: Package') {
            when { expression { env.NODE_OS == 'Darwin' } }
            steps {
                script {
                    def appBundle = "${BUILD_DIR}/KayteIDE.app"
                    def macdeployqt = "${env.QT_PATH_MACOS}/bin/macdeployqt"
                    def dmgName = "KayteIDE-macOS-universal-${env.RELEASE_CYCLE}-${env.BUILD_NUMBER}.dmg"

                    sh "test -f ${macdeployqt}"
                    sh "ci/mac/verify-universal.sh ${appBundle}/Contents/MacOS/KayteIDE"
                    sh "${macdeployqt} ${appBundle} -dmg"
                    sh "mv ${BUILD_DIR}/KayteIDE.dmg ${BUILD_DIR}/${dmgName}"
                    env.MACOS_ARTIFACT = "${BUILD_DIR}/${dmgName}"
                }
            }
            post {
                success {
                    archiveArtifacts artifacts: "${env.MACOS_ARTIFACT}", fingerprint: true
                }
            }
        }

        stage('Linux (native): Configure, Build & Package') {
            when { expression { env.NODE_OS == 'Linux' } }
            steps {
                script {
                    def installDir = "${WORKSPACE}/install/KayteIDE"
                    def tarName = "KayteIDE-linux-${env.NODE_ARCH}-${env.RELEASE_CYCLE}-${env.BUILD_NUMBER}.tar.gz"

                    sh """
                        mkdir -p ${BUILD_DIR}
                        cmake -S . -B ${BUILD_DIR} \
                            -DCMAKE_BUILD_TYPE=Release \
                            -DCMAKE_INSTALL_PREFIX=${installDir} \
                            -DCMAKE_PREFIX_PATH=${QT_PATH_LINUX}/lib/cmake
                        cmake --build ${BUILD_DIR} -j "\$(nproc)"
                        cmake --install ${BUILD_DIR}
                        tar -czf ${tarName} -C ${WORKSPACE}/install KayteIDE
                    """
                    env["LINUX_ARTIFACT_native_${env.NODE_ARCH}"] = tarName
                }
            }
            post {
                success {
                    archiveArtifacts artifacts: "${env['LINUX_ARTIFACT_native_' + env.NODE_ARCH]}", fingerprint: true
                }
            }
        }

        stage('Linux (VM builders): Build & Package [Alpine arm64/amd64 + Debian 12 arm64]') {
            when { expression { env.NODE_OS == 'Darwin' } }
            steps {
                script {
                    def branches = [:]

                    // Alpine Linux, one small VM per target architecture,
                    // managed by hand in UTM per ci/vm/README.md.
                    def alpineVms = [
                        arm64: 'kayteide-alpine-builder-arm64',
                        amd64: 'kayteide-alpine-builder-amd64',
                    ]
                    alpineVms.each { arch, vmName ->
                        branches["alpine-${arch}"] = {
                            withEnv(["VM_NAME=${vmName}"]) {
                                def outDir = "linux-vm-out-alpine-${arch}"
                                def tarName = "KayteIDE-linux-alpine-${arch}-${env.RELEASE_CYCLE}-${env.BUILD_NUMBER}.tar.gz"
                                sh 'ci/vm/start-vm.sh'
                                try {
                                    sh "ci/vm/build-in-vm.sh ${outDir}"
                                } finally {
                                    sh 'ci/vm/stop-vm.sh || true'
                                }
                                sh "tar -czf ${tarName} -C ${outDir} ."
                                archiveArtifacts artifacts: "${tarName}", fingerprint: true
                                env["LINUX_ARTIFACT_alpine_${arch}"] = tarName
                            }
                        }
                    }

                    // Debian 12 (bookworm), arm64 only, besides Alpine -
                    // managed by `vagrant` + the `vagrant-qemu` provider
                    // instead of UTM, per ci/vagrant/README.md.
                    branches['debian12-arm64'] = {
                        def outDir = 'linux-vm-out-debian12-arm64'
                        def tarName = "KayteIDE-linux-debian12-arm64-${env.RELEASE_CYCLE}-${env.BUILD_NUMBER}.tar.gz"
                        sh 'ci/vagrant/start-vm.sh'
                        try {
                            sh "ci/vagrant/build-in-vm.sh ${outDir}"
                        } finally {
                            sh 'ci/vagrant/stop-vm.sh || true'
                        }
                        sh "tar -czf ${tarName} -C ${outDir} ."
                        archiveArtifacts artifacts: "${tarName}", fingerprint: true
                        env['LINUX_ARTIFACT_debian_arm64'] = tarName
                    }

                    // Each VM defaults to 2 vCPU / 2GB RAM, so all three fit
                    // comfortably alongside the native macOS build (see the
                    // resource notes in ci/vm/README.md and
                    // ci/vagrant/README.md).
                    parallel branches
                }
            }
        }

        stage('Deploy') {
            when {
                expression {
                    env.MACOS_ARTIFACT || env.LINUX_ARTIFACT_alpine_arm64 || env.LINUX_ARTIFACT_alpine_amd64 ||
                    env.LINUX_ARTIFACT_debian_arm64 || env.LINUX_ARTIFACT_native_arm64 || env.LINUX_ARTIFACT_native_amd64
                }
            }
            steps {
                echo "Deployment stage: uploading artifacts for release cycle ${env.RELEASE_CYCLE}"
                script {
                    // Requires an 'aws-credentials' Jenkins credential ID.
                    def artifacts = [
                        env.MACOS_ARTIFACT,
                        env.LINUX_ARTIFACT_alpine_arm64,
                        env.LINUX_ARTIFACT_alpine_amd64,
                        env.LINUX_ARTIFACT_debian_arm64,
                        env.LINUX_ARTIFACT_native_arm64,
                        env.LINUX_ARTIFACT_native_amd64,
                    ].findAll { it }
                    artifacts.each { path ->
                        def fileName = path.tokenize('/').last()
                        withAWS(credentials: 'aws-credentials') {
                            sh "aws s3 cp '${path}' s3://your-release-bucket/kayteide/${env.RELEASE_CYCLE}/${fileName}"
                        }
                    }
                }
            }
        }
    }

    post {
        always {
            cleanWs()
        }
        success {
            echo 'KayteIDE build successful!'
        }
        failure {
            echo 'KayteIDE build failed!'
        }
    }
}
