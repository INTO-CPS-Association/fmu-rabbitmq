#!groovy

pipeline {
    agent any

    options { buildDiscarder(logRotator(numToKeepStr: '5')) }

    stages {
        stage('Checkout') {
            steps {
              checkout scm

     sh label: '', script: 'git submodule update --init --recursive'

         }
        }

      stage('Compile Dependencies') {
            steps {
                script {

                    dir("thirdparty") {
                        sh label: '', script: './build_dependencies.sh'
                    }

                }
            }
        }



        stage('Compile') {
            steps {
                script {

                    sh label: '', script: './scripts/build_linux-64.sh'

                }
            }
        }

        stage('Test') {
                    steps {

                    sh label: '', script: 'cp rabbitmq-fmu/modelDescription.xml build/linux-x64/rabbitmq-fmu/'
                    sh label: '', script: 'cp rabbitmq-fmu/data* build/linux-x64/rabbitmq-fmu/'
                    dir("build/linux-x64/rabbitmq-fmu") {
                        script {

                            sh label: '', script: './unit-test-rabbitmq-fmu'

                        }
                    }
                }
                }
   }
}

