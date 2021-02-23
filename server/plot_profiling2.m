function plot_profiling2(filename)
    %file = 'oldmain_1ms.txt';
    %to handle big numbers for visualization
    format long g;
    
    %getting data from the file
    %Data looks like: GI: 0:+243, 1:345, ..., 5:3453
    FID = fopen(strcat(filename,'.txt'));
    data = textscan(FID,'%s');
    fclose(FID);
    stringData = string(data{:});
    result = find(stringData=='GI:' | stringData=='HE:');
    stringData(result) = [];
    data = reshape(stringData, 6, [])'
    data = regexprep(data,'\w*:','');
    data = regexprep(data,'+','')
    data = str2double(data)
    
    %plot, take the log10 since some value are exponentially bigger and don't fit
    %in the plot.
    h = bar(log10(data));
    set(gca,'yscale','log')
    set(h, {'DisplayName'}, {'0','1','2', '3' , '4' , '5'}');
    
    % Legend will show names for each color
    legend();
    xlabel('dostep() cycle') 
    ylabel('log10(time in ms)') 
    grid
    saveas(gcf,filename,'png')

end