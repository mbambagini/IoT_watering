% plot temperature of the last 7 days

readChannelID = 000000;
fields = [1, 4, 5];
readAPIKey = 'XXXXXXXXXXXXXXXX';

% read data
[values, time] = thingSpeakRead(readChannelID, 'Fields', fields,...
                                               'NumDays', 7,...
                                               'ReadKey', readAPIKey);
if ~isempty(values)
    node = values(:, 1);
    temp = values(:, 2);
    vali = values(:, 3);

    % get only valid data
    validIdxs = vali == 1;
    node = node(validIdxs);
    temp = temp(validIdxs);
    time = time(validIdxs);

    % get data for each watering unit
    n = unique(node);
    matrix_time = cell(length(n));
    matrix_temp = cell(length(n));
    matrix_name = cell(length(n));
    for i=1:length(n)
        ids = node == n(i);
        matrix_time{i} = time(ids);
        matrix_temp{i} = temp(ids);
        matrix_name{i} = ['Watering unit #' int2str(n(i))];
    end

    % plot
    hold on
    p = [];
    for i=1:length(n)
        p(end+1) = plot(matrix_time{i}, matrix_temp{i},...
                    'LineStyle', '-', 'Marker', '*');
    end
    legend(p, matrix_name);
    hold off
end

