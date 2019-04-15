%Load and prepare data

%	Choosing the first set of wavelengths selects the CIE 1931 standard.  The second selects 2006.

wavelengths = 380:5:780;
%wavelengths = 390:1:780;

xyzbar_380_5_780 = csvread('cie1931-xyzbar-380+5+780.csv');
xyzbar_390_1_830 = csvread('cie2006-xyzbar-390+1+830.csv');

d65_300_5_780 = csvread('d65-300+5+780.csv');

xbar1931 = resample_spectrum( xyzbar_380_5_780(:,1), 380:5:780, wavelengths );
ybar1931 = resample_spectrum( xyzbar_380_5_780(:,2), 380:5:780, wavelengths );
zbar1931 = resample_spectrum( xyzbar_380_5_780(:,3), 380:5:780, wavelengths );
xbar2006 = resample_spectrum( xyzbar_390_1_830(:,1), 390:1:830, wavelengths );
ybar2006 = resample_spectrum( xyzbar_390_1_830(:,2), 390:1:830, wavelengths );
zbar2006 = resample_spectrum( xyzbar_390_1_830(:,3), 390:1:830, wavelengths );
xyzbar1931 = [xbar1931,ybar1931,zbar1931];
xyzbar2006 = [xbar2006,ybar2006,zbar2006];

if wavelengths(1) == 380
	xyzbar = xyzbar1931;
else
	xyzbar = xyzbar2006;
end

D65 = resample_spectrum( d65_300_5_780, 300:5:780, wavelengths );



%Calculate RGB-to-XYZ and XYZ-to-RGB matrices.

matr_RGBtoXYZ = calc_matr_RGBtoXYZ( ...
	[ 0.64 0.33 ], [ 0.30 0.60 ], [ 0.15 0.06 ], ...
	calc_XYZ(xyzbar,D65) ...
);
matr_XYZtoRGB = inv(matr_RGBtoXYZ);



%Solve for the spectral basis.
%	See comments within and paper for details.

[ basis_r, basis_g, basis_b ] = solve_basis(wavelengths,xyzbar,matr_RGBtoXYZ,D65);

%	Check basis

check_basis( xyzbar, matr_XYZtoRGB, basis_r,basis_g,basis_b, D65 );

%	Save basis

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
plot(wavelengths,basis_r,                'r-');
plot(wavelengths,basis_g,                'g-');
plot(wavelengths,basis_b,                'b-');
plot(wavelengths,basis_r+basis_g+basis_b,'k:');
hold off
xlim([wavelengths(1) wavelengths(end)]);
ylim([0 1]);



%Functions

function [result] = resample_spectrum( spectrum, xs0, xs1 )
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
	%Size of system

	n = size(wavelengths,2);



	%Equality constraint: ensure each reflection spectrum maps to the corresponding ℓRGB tristimulus
	%	value (e.g., when illuminated with D65, the red spectrum should produce <1,0,0>).

	A_eq = zeros(9,3*n);

	row0 = ( whitept .* xyzbar(:,1) )';
	row1 = ( whitept .* xyzbar(:,2) )';
	row2 = ( whitept .* xyzbar(:,3) )';

	A_eq( 1,     1:  n ) = row0;
	A_eq( 2,     1:  n ) = row1;
	A_eq( 3,     1:  n ) = row2;

	A_eq( 4,   n+1:2*n ) = row0;
	A_eq( 5,   n+1:2*n ) = row1;
	A_eq( 6,   n+1:2*n ) = row2;

	A_eq( 7, 2*n+1:3*n ) = row0;
	A_eq( 8, 2*n+1:3*n ) = row1;
	A_eq( 9, 2*n+1:3*n ) = row2;

	b_eq = [
		matr_RGBtoXYZ * [ 1 0 0 ]' ;
		matr_RGBtoXYZ * [ 0 1 0 ]' ;
		matr_RGBtoXYZ * [ 0 0 1 ]'
	];



	%Ensure that the per-wavelength sum of the three reflection spectra is no larger than 1.  This,
	%	along with the upper and lower bounds, below, ensures all linear combinations with weights
	%	in [0,1] produce valid reflectances.

	A_leq = [ eye(n), eye(n), eye(n) ];

	b_leq = ones(n,1);



	%%Save constraints, for debugging

	%dlmwrite('A_eq.txt',A_eq);
	%dlmwrite('b_eq.txt',b_eq);
	%dlmwrite('A_leq.txt',A_leq);
	%dlmwrite('b_leq.txt',b_leq);



	%Ensure that each spectrum has per-wavelength values in [0,1].

	lb=zeros(3*n,1); ub=ones(3*n,1);



	%Solve system

	%	Precondition with a simple program just to get something feasible.
	%		https://www.mathworks.com/help/optim/ug/linprog.html

	f = ones(3*n,1);
	rgb = linprog( f, A_leq,b_leq, A_eq,b_eq, lb,ub );

	%%		https://www.mathworks.com/help/optim/ug/quadprog.html
	%H=eye(3*n); f=ones(3*n,1);
	%rgb = quadprog( H,f, A_leq,b_leq, A_eq,b_eq, lb,ub );

	%	Use a constrained optimization.  There are many possible objective functions, but the
	%		reasonable ones all seem to produce surprisingly similar spectra.  The solve takes a
	%		while, mainly because the tolerance is cranked very fine.  Comment the three tolerance
	%		values to run much faster (this produces spectra using the default tolerances, which are
	%		basically just as good in-practice, if you just want to see how it works).

	p = inf;

	%fn = @(rgb) norm(rgb,p);
	fn = @(rgb) norm(rgb(1:n),p) + norm(rgb(n+1:2*n),p) + norm(rgb(2*n+1:3*n),p);
	%fn = @(rgb) norm(rgb(2*n+1:3*n),p);

	%fn = @(rgb) norm(diff(rgb),p);
	%fn = @(rgb) norm(diff(rgb(1:n)),p) + norm(diff(rgb(n+1:2*n)),p) + norm(diff(rgb(2*n+1:3*n)),p);

	options = optimoptions('fmincon');

	options.ConstraintTolerance = 1e-11; %default 1e-6
	options.OptimalityTolerance = 1e-10; %default 1e-6
	options.StepTolerance       = 1e-14; %default 1e-6 or 1e-10
	options.MaxFunctionEvaluations = 1000000;
	options.MaxIterations          = 10000;

	%options.Display = 'iter';
	%options.Algorithm = 'sqp';

	rgb = fmincon( fn, rgb, A_leq,b_leq, A_eq,b_eq, lb,ub, [], options );



	%Return the solved spectral basis

	r = rgb(     1:  n, 1 );
	g = rgb(   n+1:2*n, 1 );
	b = rgb( 2*n+1:3*n, 1 );
end

function [] = check_basis( xyzbar, matr_XYZtoRGB, basis_r,basis_g,basis_b, D65 )
	RGB_d65r   = ( matr_XYZtoRGB * calc_XYZ(xyzbar, basis_r                 .*D65) );
	RGB_d65g   = ( matr_XYZtoRGB * calc_XYZ(xyzbar,         basis_g         .*D65) );
	RGB_d65b   = ( matr_XYZtoRGB * calc_XYZ(xyzbar,                 basis_b .*D65) );

	RGB_d65rgb = ( matr_XYZtoRGB * calc_XYZ(xyzbar,(basis_r+basis_g+basis_b).*D65) );

	%RGB_d65rgb'

	basis_residual_for_R_G_B_and_RGB = [ RGB_d65r, RGB_d65g, RGB_d65b, RGB_d65rgb ] - [
		1 0 0 1 ;
		0 1 0 1 ;
		0 0 1 1
	]
end
