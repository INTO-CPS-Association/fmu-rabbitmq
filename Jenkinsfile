#!groovy

pipeline {
    agent any

    options { buildDiscarder(logRotator(numToKeepStr: '5')) }

    stages {

        stage('Checkout') {
            steps {
                cleanWs()
                checkout scm

                sh label: '', script: 'git submodule update --init'

            }
        }

        stage('XCompile native code') {

            parallel {
                stage('Xcompile darwin:') {
                    steps {
                        sh "./scripts/darwin64_build.sh"
                    }
                }


                stage('Xcompile linux32') {
                    steps {
                        sh "./scripts/linux32_build.sh"
                    }
                }
                stage('Xcompile linux64') {
                    steps {
                        sh "./scripts/linux64_build.sh"
/*                        dir("build/linux-x64") {
                           script {
                               sh label: '', script: './linux-x64-dockcross ./rabbitmq-core/unit-test-rabbitmq-core'
                           }
                        }*/
/*                        dir("build/linux-x64/rabbitmq-fmu") {
                            script {
                                sh label: '', script: './../linux-x64-dockcross ./unit-test-rabbitmq'
                            }
                        }*/
                    }
                }

                stage('Xcompile win32') {
                    steps {
                        sh "./scripts/win32_build.sh"
                    }
                }
                 stage('Xcompile win64') {
                    steps {
                        sh "./scripts/win64_build.sh"
                     }
                 }
            }
        }



        stage('Pack FMU') {
            steps {
                script {
		   sh label: 'packing', script: './scripts/pack_fmu.sh'
                }
            }
        }


        stage('Publish') {
            steps {
                sshPublisher(
                        publishers: [sshPublisherDesc(
                                configName: 'overture.au.dk - into-cps',
                                transfers: [sshTransfer(
                                        cleanRemote: false,
                                        excludes: '',
                                        execCommand: "~/update-latest.sh web/into-cps/rabbitmqfmu/${BRANCH_NAME}",
                                        execTimeout: 120000,
                                        flatten: true,
                                        makeEmptyDirs: false,
                                        noDefaultExcludes: false,
                                        patternSeparator: '[, ]+',
                                        remoteDirectory: '\'rabbitmqfmu/${BRANCH_NAME}/Build-${BUILD_NUMBER}_\'yyyy-MM-dd_HH-mm',
                                        remoteDirectorySDF: true,
                                        removePrefix: '',
                                        sourceFiles: 'build/*.fmu')],
                                usePromotionTimestamp: false,
                                useWorkspaceInPromotion: false,
                                verbose: false)])
            }
        }
    }
}

