clear;
file = 'newmain_1ms.txt';
%to handle big numbers for visualization
format long g;
%getting data from the file
fileID = fopen(file,'r');
formatSpec = '%f %f';
sizeA = [6 Inf];
A = fscanf(fileID,formatSpec,sizeA);
A = A';
%plot, take the log10 since some value are exponentially bigger and don't fit
%in the plot.
h = bar(log10(A));
set(gca,'yscale','log')
set(h, {'DisplayName'}, {'0','1','2', '3' , '4' , '5'}');
% Legend will show names for each color
legend();
xlabel('cycle in dostep()') 
ylabel('log10(time in ms)') 
grid
fclose(fileID);