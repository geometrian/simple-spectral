%Load data

xyzbar_380_5_780 = csvread('cie1931-xyzbar-380+5+780.csv');
xyzbar_390_1_830 = csvread('cie2006-xyzbar-390+1+830.csv');

d65_300_5_780 = csvread('d65-300+5+780.csv');



%wavelengths = 380:5:780;
wavelengths = 300:1:830;

xbar1931 = myresample( xyzbar_380_5_780(:,1), 380:5:780, wavelengths );
ybar1931 = myresample( xyzbar_380_5_780(:,2), 380:5:780, wavelengths );
zbar1931 = myresample( xyzbar_380_5_780(:,3), 380:5:780, wavelengths );
xbar2006 = myresample( xyzbar_390_1_830(:,1), 390:1:830, wavelengths );
ybar2006 = myresample( xyzbar_390_1_830(:,2), 390:1:830, wavelengths );
zbar2006 = myresample( xyzbar_390_1_830(:,3), 390:1:830, wavelengths );
xyzbar1931 = [xbar1931,ybar1931,zbar1931];
xyzbar2006 = [xbar2006,ybar2006,zbar2006];

%xyzbar = xyzbar1931;
xyzbar = xyzbar2006;

D65 = myresample( d65_300_5_780, 300:5:780, wavelengths );



matr_RGBtoXYZ = calc_matr_RGBtoXYZ( ...
    [ 0.64 0.33 ], [ 0.30 0.60 ], [ 0.15 0.06 ], ...
    calc_XYZ(xyzbar,D65) ...
);
matr_XYZtoRGB = inv(matr_RGBtoXYZ);

[ basis_r, basis_g, basis_b ] = solve_basis(wavelengths,xyzbar,matr_RGBtoXYZ,D65);



%Check

RGB_d65r = ( matr_XYZtoRGB * calc_XYZ(xyzbar,basis_r.*D65) )'
RGB_d65g = ( matr_XYZtoRGB * calc_XYZ(xyzbar,basis_g.*D65) )'
RGB_d65b = ( matr_XYZtoRGB * calc_XYZ(xyzbar,basis_b.*D65) )'
residual = [ RGB_d65r', RGB_d65g', RGB_d65b' ] - eye(3)



%Save

writematrix([ basis_r, basis_g, basis_b ],'basis_rgb.csv');



%Plots

close all

f1 = figure(1);
title('D65 Standard Illuminant');
plot( wavelengths, D65' );
xlim([wavelengths(1) wavelengths(end)]);
ylim([0 inf]);

f2 = figure(2);
title('CIE Standard Observer Functions');
hold on
plot(wavelengths,xbar2006,'r-');
plot(wavelengths,ybar2006,'g-');
plot(wavelengths,zbar2006,'b-');
plot(wavelengths,xbar1931,'r:');
plot(wavelengths,ybar1931,'g:');
plot(wavelengths,zbar1931,'b:');
hold off
xlim([wavelengths(1) wavelengths(end)]);
ylim([0 inf]);

f3 = figure(3);
title('Spectral Basis');
hold on
plot(wavelengths,basis_r,'r-');
plot(wavelengths,basis_g,'g-');
plot(wavelengths,basis_b,'b-');
hold off
xlim([wavelengths(1) wavelengths(end)]);
ylim([0 1]);



%Functions

function [result] = myresample( spectrum, xs0, xs1 )
    result = interp1( xs0,spectrum', xs1, 'spline', 0.0 )';
end

function [XYZ] = calc_XYZ(xyzbar,spectrum)
    XYZ = xyzbar' * spectrum;
end

function [M] = calc_matr_RGBtoXYZ( xy_r, xy_g, xy_b, XYZ_W )
    %http://brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html

    x_rgb = [ xy_r(1) xy_g(1) xy_b(1) ]';
    y_rgb = [ xy_r(2) xy_g(2) xy_b(2) ]';
    
    X_rgb = x_rgb ./ y_rgb;
    Y_rgb = [ 1 1 1 ]';
    Z_rgb = ( [ 1 1 1 ]' - x_rgb - y_rgb ) ./ y_rgb;
    
    S_rgb = inv([ X_rgb' ; Y_rgb' ; Z_rgb' ]) * XYZ_W;
    
    M = [
        ( S_rgb .* X_rgb )' ;
        ( S_rgb .* Y_rgb )' ;
        ( S_rgb .* Z_rgb )'
    ];
end

function [r,g,b] = solve_basis(wavelengths,xyzbar,matr_RGBtoXYZ,whitept)
    n = size(wavelengths,2);
    
    
    A_eq = zeros(9,3*n);
    
    row0 = ( whitept .* xyzbar(:,1) )';
    row1 = ( whitept .* xyzbar(:,2) )';
    row2 = ( whitept .* xyzbar(:,3) )';
    
    A_eq( 1, 1:n       ) = row0;
    A_eq( 2, 1:n       ) = row1;
    A_eq( 3, 1:n       ) = row2;
   
    A_eq( 4, n+1:2*n   ) = row0;
    A_eq( 5, n+1:2*n   ) = row1;
    A_eq( 6, n+1:2*n   ) = row2;
   
    A_eq( 7, 2*n+1:3*n ) = row0;
    A_eq( 8, 2*n+1:3*n ) = row1;
    A_eq( 9, 2*n+1:3*n ) = row2;
    
    b_eq = [
        matr_RGBtoXYZ*[1;0;0] ;
        matr_RGBtoXYZ*[0;1;0] ;
        matr_RGBtoXYZ*[0;0;1]
    ];
    
    
    A_leq = [ eye(n), eye(n), eye(n) ];
    
    b_leq = ones(n,1);
    
    
    %dlmwrite('A_eq.txt',A_eq);
    %dlmwrite('b_eq.txt',b_eq);
    %dlmwrite('A_leq.txt',A_leq);
    %dlmwrite('b_leq.txt',b_leq);
    
    
    lb=zeros(3*n,1); ub=ones (3*n,1);
    
    %%https://www.mathworks.com/help/optim/ug/quadprog.html
    %H=eye(3*n); f=ones(3*n,1);
    %rgb = quadprog( H,f, A_leq,b_leq, A_eq,b_eq, lb,ub );
    
    %https://www.mathworks.com/help/optim/ug/linprog.html
    f = ones(3*n,1);
    rgb = linprog( f, A_leq,b_leq, A_eq,b_eq, lb,ub );
    
    fun = @(rgb) norm(diff(rgb));
    options = optimoptions('fmincon');
    options.ConstraintTolerance = 1e-14;
    options.MaxFunctionEvaluations = 50000;
    rgb = fmincon( fun, rgb, A_leq,b_leq, A_eq,b_eq, lb,ub, [], options );
    
    
    r = rgb(     1:  n, 1 );
    g = rgb(   n+1:2*n, 1 );
    b = rgb( 2*n+1:3*n, 1 );
end














