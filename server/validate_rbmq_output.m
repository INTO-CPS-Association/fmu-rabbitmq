clear;
%Get data that is sent to the rabbitmq

ground_truth = readtable('ur_robot.csv');

%Get data logged by the intocps app

processed_data = readtable('outputs.csv');

nr_datapoints = size(processed_data,1)

%find columns to diff
col_gt = find(strcmp(ground_truth.Properties.VariableNames, 'actual_current_0'), 1)

col_pd = find(strcmp(processed_data.Properties.VariableNames, 'x_FMU__FMUInstance_actual_current_0'), 1)

%get difference

difference = processed_data{:, col_pd} - ground_truth{1:nr_datapoints, col_gt}

figure
hold on
%plot the difference
plot(processed_data{:, 1}, difference)

plot(processed_data{:, 1}, processed_data{:, col_pd})
figure
plot(ground_truth{1:nr_datapoints, 1}, ground_truth{1:nr_datapoints, col_gt})