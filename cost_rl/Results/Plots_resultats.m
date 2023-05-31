

folderPath = '6BG';
files = dir(fullfile(folderPath, '*.csv'));
dataStruct = struct(); 

value_t50 = zeros(numel(files),1); %%Let's just store the 'closest value' and initialize all cum_rewards to an empty array
value_t200 = value_t50;
value_t400  = value_t50;


for i = 1:numel(files)

    fileName = files(i).name; 
    filePath = fullfile(folderPath, fileName);
    dataTable = readtable(filePath);
    
    % Store the table in the struct
    string = sprintf('%s%d','A',num2str(i));
    disp(string)
    dataStruct.(string) = dataTable; 
    
    % Get the column of interest
    columnData = dataTable.simtime;

    % Calculate the absolute difference between each element of the column and 50
    diffValues_50 = abs(columnData - 50);
    diffValues_200 = abs(columnData - 200);
    diffValues_400 = abs(columnData - 400);
    % Find the index of the minimum difference value
    [minDiff, rowIndex_50] = min(diffValues_50);
    [minDiff, rowIndex_200] = min(diffValues_200);
    [minDiff, rowIndex_400] = min(diffValues_400);

    % Get the row with the closest value to 50
    closestRow50 = dataTable(rowIndex_50, :);
    closestRow200 = dataTable(rowIndex_200, :);
    closestRow400 = dataTable(rowIndex_400, :);

    value_t50(i) = closestRow50.cum_reward;
    value_t200(i) = closestRow200.cum_reward;
    value_t400(i) = closestRow400.cum_reward;
end

value_t50 = double(value_t50);
value_t200 = double(value_t200);
value_t400 = double(value_t400);


data = horzcat(value_t50, value_t200, value_t400);
boxchart([value_t50, value_t200, value_t400]);
shg





% 
% 
% 
% data = {value_t50, value_t200, value_t400};
% boxplot(data, 'Labels', {'value_t50', 'value_t51', 'value_t52'});
% % group = [ones(size(value_t50)); 2 * ones(size(value_t200)); 3 * ones(size(value_t400))];
% % figure
% boxplot([value_t50; value_t200; value_t400], group )
% set(gca,'XTickLabel',{'A','B','C'})