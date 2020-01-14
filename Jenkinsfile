#!groovy

pipeline {
    agent any

    options { buildDiscarder(logRotator(numToKeepStr: '5')) }

    stages {

        stage('Compile native code X') {

        	parallel  {
        		node('Xcompile darwin:') {
        			checkout scm
        			sh 'git submodule update --init'
        			sh "./script/darwin64_build.sh"
        			stash includes: '/work/build/install/rabbitmqfmu/binaries/**/.dylib', name: 'rabbitmqfmu-darwin'
        		}
        	}: {
        		node('Xcompile linux32') {
         			checkout scm
           			sh 'git submodule update --init'
           			sh "./script/linux32_build.sh"
           			stash includes: '/work/build/install/rabbitmqfmu/binaries/**/.so', name: 'rabbitmqfmu-linux32'
        		}
        	}: {
        		node('Xcompile linux64') {
         			checkout scm
           			sh 'git submodule update --init'
           			sh "./script/linux64_build.sh"
           			stash includes: '/work/build/install/rabbitmqfmu', name: 'rabbitmqfmu-linux64'
           			dir("build/linux-x64/rabbitmq-fmu") {
                        script {
                            sh label: '', script: './unit-test-rabbitmq-fmu'
                        }
                    }
        		}
        	}: {
        		node('Xcompile win32') {
                	checkout scm
           			sh 'git submodule update --init'
 //          			sh "./script/win32_build.sh"
 //          			stash includes: '/work/build/install/rabbitmqfmu/binaries/**/.dll', name: 'rabbitmqfmu-win32'
        		}
        	}: {
        		node('Xcompile win64') {
                	checkout scm
           			sh 'git submodule update --init'
           			sh "./script/win64_build.sh"
           			stash includes: '/work/build/install/rabbitmqfmu/binaries/**/.dll', name: 'rabbitmqfmu-win64'
        		}
        	}
        }



//         stage('Checkout') {
//             steps {
//          //   cleanWs()
//               checkout scm
//
//      sh label: '', script: 'git submodule update --init'
//
//          }
//         }

//       stage('Compile Dependencies') {
//             steps {
//                 script {
//
//                     dir("thirdparty") {
//                         sh label: '', script: './build_dependencies.sh'
//                     }
//
//                 }
//             }
//         }

//         stage('Test') {
//                     steps {
//                     dir("build/linux-x64/rabbitmq-fmu") {
//                         script {
//                             sh label: '', script: './unit-test-rabbitmq-fmu'
//                         }
//                     }
//                 }
//         }

        stage('Pack FMU') {
            steps {
                script {
                   //copy back all native libraries
                    unstash 'rabbitmqfmu-linux64'
                    unstash 'rabbitmqfmu-darwin'
                    unstash 'rabbitmqfmu-linux32'

                //	unstash 'rabbitmqfmu-win32'
                    unstash 'rabbitmqfmu-win64'

		             dir("build/install/rabbitmqfmu") {
                            sh label: '', script: 'zip -r ../rabbitmqfmu.fmu .'
                    }



                }
            }
        }
   }
}

