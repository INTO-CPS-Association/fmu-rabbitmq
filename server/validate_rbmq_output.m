clear;
format long

maxage = 2
outputs = 'outputs_maxage2000ms.csv';
gt_data = 'ur_robot.csv';
%Get data that is sent to the rabbitmq
ground_truth = readtable(gt_data);
seqnocol_gt = find(strcmp(ground_truth.Properties.VariableNames, 'seqno'), 1);
seqno_gt = ground_truth{:, seqnocol_gt};
%Get data logged by the intocps app

processed_data = readtable(outputs);

% find noseq outputted by rabbitmq outputs.csv
seqnocol = find(strcmp(processed_data.Properties.VariableNames, 'x_FMU__FMUInstance_seqno'), 1);
seqno = processed_data{:, seqnocol};
% NOTE the sequence number starts from 0
% extract data from ur_robot.csv based on the seq number.
col_gt = find(strcmp(ground_truth.Properties.VariableNames, 'actual_current_0'), 1);
gt_subset = ground_truth{seqno+1, col_gt};

%find column to diff
col_pd = find(strcmp(processed_data.Properties.VariableNames, 'x_FMU__FMUInstance_actual_current_0'), 1);

%get difference
difference = processed_data{:, col_pd} - gt_subset;
figure
hold on
%plot the difference, and the data 

title_part = ['in/out data based on seq number, maxage = ' num2str(maxage) 's']
title(title_part)
plot(processed_data{:, 1}, difference)

plot(processed_data{:, 1}, processed_data{:, col_pd})

plot(processed_data{:, 1}, gt_subset, '--')

legend('diff','rbmq output','ground truth')
hold off

% Plot the sequence numbers of the active message for each timestep in the
% simulation

% first convert the sim time to the gt time
sim_time = processed_data{:, 1} + ground_truth{seqno(1)+1, 1};

figure
hold on
title_part = ['Sequence numbers of messages, maxage = ' num2str(maxage) 's']
title(title_part)
plot(sim_time, seqno, '*')

plot(ground_truth{:, 1}, ground_truth{:, seqnocol_gt}, 'x')

xlim([sim_time(1) sim_time(end)])
legend('sim\_seq','gt\_seq')
set(gca,'xminortick','on') 
grid on

hold off
