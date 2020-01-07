#!groovy

pipeline {
    agent any

    options { buildDiscarder(logRotator(numToKeepStr: '5')) }

    stages {
        stage('Checkout') {
            steps {
                checkout([
        $class: 'GitSCM',
        branches: scm.branches,
        doGenerateSubmoduleConfigurations: true,
        extensions: scm.extensions + [[$class: 'SubmoduleOption', parentCredentials: true]],
        userRemoteConfigs: scm.userRemoteConfigs
    ])
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

