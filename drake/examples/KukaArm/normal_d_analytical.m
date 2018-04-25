function [normal, d, mu] = normal_d_analytical(q)
theta_1 = q(1);theta_2 = q(2);theta_3 = q(3);theta_4 = q(4);theta_5 = q(5);theta_6 = q(6);theta_7 = q(7);theta_8 = q(8);
obj_x = q(9); obj_y = q(10); obj_z = q(11); obj_yaw = q(12); obj_pitch = q(13); obj_roll = q(14);

R_world_to_B = rpy2rotmat([obj_yaw;obj_pitch;obj_roll]);

fr1 = [(21*cos(theta_1)*sin(theta_2))/50 + (407*cos(theta_6)*(sin(theta_4)*(sin(theta_1)*sin(theta_3) - cos(theta_1)*cos(theta_2)*cos(theta_3)) + cos(theta_1)*cos(theta_4)*sin(theta_2)))/2000 - (cos(theta_7)*(sin(theta_6)*(sin(theta_4)*(sin(theta_1)*sin(theta_3) - cos(theta_1)*cos(theta_2)*cos(theta_3)) + cos(theta_1)*cos(theta_4)*sin(theta_2)) + cos(theta_6)*(cos(theta_5)*(cos(theta_4)*(sin(theta_1)*sin(theta_3) - cos(theta_1)*cos(theta_2)*cos(theta_3)) - cos(theta_1)*sin(theta_2)*sin(theta_4)) + sin(theta_5)*(cos(theta_3)*sin(theta_1) + cos(theta_1)*cos(theta_2)*sin(theta_3)))))/100 + (cos(theta_7)*(sin(theta_5)*(cos(theta_4)*(sin(theta_1)*sin(theta_3) - cos(theta_1)*cos(theta_2)*cos(theta_3)) - cos(theta_1)*sin(theta_2)*sin(theta_4)) - cos(theta_5)*(cos(theta_3)*sin(theta_1) + cos(theta_1)*cos(theta_2)*sin(theta_3))))/25 + (sin(theta_7)*(sin(theta_6)*(sin(theta_4)*(sin(theta_1)*sin(theta_3) - cos(theta_1)*cos(theta_2)*cos(theta_3)) + cos(theta_1)*cos(theta_4)*sin(theta_2)) + cos(theta_6)*(cos(theta_5)*(cos(theta_4)*(sin(theta_1)*sin(theta_3) - cos(theta_1)*cos(theta_2)*cos(theta_3)) - cos(theta_1)*sin(theta_2)*sin(theta_4)) + sin(theta_5)*(cos(theta_3)*sin(theta_1) + cos(theta_1)*cos(theta_2)*sin(theta_3)))))/25 - (407*sin(theta_6)*(cos(theta_5)*(cos(theta_4)*(sin(theta_1)*sin(theta_3) - cos(theta_1)*cos(theta_2)*cos(theta_3)) - cos(theta_1)*sin(theta_2)*sin(theta_4)) + sin(theta_5)*(cos(theta_3)*sin(theta_1) + cos(theta_1)*cos(theta_2)*sin(theta_3))))/2000 + (sin(theta_7)*(sin(theta_5)*(cos(theta_4)*(sin(theta_1)*sin(theta_3) - cos(theta_1)*cos(theta_2)*cos(theta_3)) - cos(theta_1)*sin(theta_2)*sin(theta_4)) - cos(theta_5)*(cos(theta_3)*sin(theta_1) + cos(theta_1)*cos(theta_2)*sin(theta_3))))/100 + (2*sin(theta_4)*(sin(theta_1)*sin(theta_3) - cos(theta_1)*cos(theta_2)*cos(theta_3)))/5 + (2*cos(theta_1)*cos(theta_4)*sin(theta_2))/5;
 (21*sin(theta_1)*sin(theta_2))/50 - (407*cos(theta_6)*(sin(theta_4)*(cos(theta_1)*sin(theta_3) + cos(theta_2)*cos(theta_3)*sin(theta_1)) - cos(theta_4)*sin(theta_1)*sin(theta_2)))/2000 - (cos(theta_7)*(sin(theta_5)*(cos(theta_4)*(cos(theta_1)*sin(theta_3) + cos(theta_2)*cos(theta_3)*sin(theta_1)) + sin(theta_1)*sin(theta_2)*sin(theta_4)) - cos(theta_5)*(cos(theta_1)*cos(theta_3) - cos(theta_2)*sin(theta_1)*sin(theta_3))))/25 + (cos(theta_7)*(sin(theta_6)*(sin(theta_4)*(cos(theta_1)*sin(theta_3) + cos(theta_2)*cos(theta_3)*sin(theta_1)) - cos(theta_4)*sin(theta_1)*sin(theta_2)) + cos(theta_6)*(cos(theta_5)*(cos(theta_4)*(cos(theta_1)*sin(theta_3) + cos(theta_2)*cos(theta_3)*sin(theta_1)) + sin(theta_1)*sin(theta_2)*sin(theta_4)) + sin(theta_5)*(cos(theta_1)*cos(theta_3) - cos(theta_2)*sin(theta_1)*sin(theta_3)))))/100 - (2*sin(theta_4)*(cos(theta_1)*sin(theta_3) + cos(theta_2)*cos(theta_3)*sin(theta_1)))/5 + (407*sin(theta_6)*(cos(theta_5)*(cos(theta_4)*(cos(theta_1)*sin(theta_3) + cos(theta_2)*cos(theta_3)*sin(theta_1)) + sin(theta_1)*sin(theta_2)*sin(theta_4)) + sin(theta_5)*(cos(theta_1)*cos(theta_3) - cos(theta_2)*sin(theta_1)*sin(theta_3))))/2000 - (sin(theta_7)*(sin(theta_5)*(cos(theta_4)*(cos(theta_1)*sin(theta_3) + cos(theta_2)*cos(theta_3)*sin(theta_1)) + sin(theta_1)*sin(theta_2)*sin(theta_4)) - cos(theta_5)*(cos(theta_1)*cos(theta_3) - cos(theta_2)*sin(theta_1)*sin(theta_3))))/100 - (sin(theta_7)*(sin(theta_6)*(sin(theta_4)*(cos(theta_1)*sin(theta_3) + cos(theta_2)*cos(theta_3)*sin(theta_1)) - cos(theta_4)*sin(theta_1)*sin(theta_2)) + cos(theta_6)*(cos(theta_5)*(cos(theta_4)*(cos(theta_1)*sin(theta_3) + cos(theta_2)*cos(theta_3)*sin(theta_1)) + sin(theta_1)*sin(theta_2)*sin(theta_4)) + sin(theta_5)*(cos(theta_1)*cos(theta_3) - cos(theta_2)*sin(theta_1)*sin(theta_3)))))/25 + (2*cos(theta_4)*sin(theta_1)*sin(theta_2))/5;
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      (21*cos(theta_2))/50 + (2*cos(theta_2)*cos(theta_4))/5 - (cos(theta_7)*(sin(theta_5)*(cos(theta_2)*sin(theta_4) - cos(theta_3)*cos(theta_4)*sin(theta_2)) - cos(theta_5)*sin(theta_2)*sin(theta_3)))/25 + (407*sin(theta_6)*(cos(theta_5)*(cos(theta_2)*sin(theta_4) - cos(theta_3)*cos(theta_4)*sin(theta_2)) + sin(theta_2)*sin(theta_3)*sin(theta_5)))/2000 - (sin(theta_7)*(sin(theta_5)*(cos(theta_2)*sin(theta_4) - cos(theta_3)*cos(theta_4)*sin(theta_2)) - cos(theta_5)*sin(theta_2)*sin(theta_3)))/100 + (407*cos(theta_6)*(cos(theta_2)*cos(theta_4) + cos(theta_3)*sin(theta_2)*sin(theta_4)))/2000 + (cos(theta_7)*(cos(theta_6)*(cos(theta_5)*(cos(theta_2)*sin(theta_4) - cos(theta_3)*cos(theta_4)*sin(theta_2)) + sin(theta_2)*sin(theta_3)*sin(theta_5)) - sin(theta_6)*(cos(theta_2)*cos(theta_4) + cos(theta_3)*sin(theta_2)*sin(theta_4))))/100 - (sin(theta_7)*(cos(theta_6)*(cos(theta_5)*(cos(theta_2)*sin(theta_4) - cos(theta_3)*cos(theta_4)*sin(theta_2)) + sin(theta_2)*sin(theta_3)*sin(theta_5)) - sin(theta_6)*(cos(theta_2)*cos(theta_4) + cos(theta_3)*sin(theta_2)*sin(theta_4))))/25 + (2*cos(theta_3)*sin(theta_2)*sin(theta_4))/5 + 9/25];
fr1x = fr1(1);
fr1y = fr1(2);
fr1z = fr1(3);

right_normal1 = [                                                                                                                                                                                                           -(fr1z*sin(obj_pitch) - obj_z*sin(obj_pitch) - fr1x*cos(obj_roll)*cos(obj_pitch) + obj_x*cos(obj_roll)*cos(obj_pitch) - fr1y*cos(obj_pitch)*sin(obj_roll) + obj_y*cos(obj_pitch)*sin(obj_roll))/((fr1x*(cos(obj_yaw)*sin(obj_roll) - cos(obj_roll)*sin(obj_yaw)*sin(obj_pitch)) - fr1y*(cos(obj_yaw)*cos(obj_roll) + sin(obj_yaw)*sin(obj_roll)*sin(obj_pitch)) - obj_x*(cos(obj_yaw)*sin(obj_roll) - cos(obj_roll)*sin(obj_yaw)*sin(obj_pitch)) + obj_y*(cos(obj_yaw)*cos(obj_roll) + sin(obj_yaw)*sin(obj_roll)*sin(obj_pitch)) - fr1z*cos(obj_pitch)*sin(obj_yaw) + obj_z*cos(obj_pitch)*sin(obj_yaw))^2 + (fr1z*sin(obj_pitch) - obj_z*sin(obj_pitch) - fr1x*cos(obj_roll)*cos(obj_pitch) + obj_x*cos(obj_roll)*cos(obj_pitch) - fr1y*cos(obj_pitch)*sin(obj_roll) + obj_y*cos(obj_pitch)*sin(obj_roll))^2)^(1/2);
 -(fr1x*(cos(obj_yaw)*sin(obj_roll) - cos(obj_roll)*sin(obj_yaw)*sin(obj_pitch)) - fr1y*(cos(obj_yaw)*cos(obj_roll) + sin(obj_yaw)*sin(obj_roll)*sin(obj_pitch)) - obj_x*(cos(obj_yaw)*sin(obj_roll) - cos(obj_roll)*sin(obj_yaw)*sin(obj_pitch)) + obj_y*(cos(obj_yaw)*cos(obj_roll) + sin(obj_yaw)*sin(obj_roll)*sin(obj_pitch)) - fr1z*cos(obj_pitch)*sin(obj_yaw) + obj_z*cos(obj_pitch)*sin(obj_yaw))/((fr1x*(cos(obj_yaw)*sin(obj_roll) - cos(obj_roll)*sin(obj_yaw)*sin(obj_pitch)) - fr1y*(cos(obj_yaw)*cos(obj_roll) + sin(obj_yaw)*sin(obj_roll)*sin(obj_pitch)) - obj_x*(cos(obj_yaw)*sin(obj_roll) - cos(obj_roll)*sin(obj_yaw)*sin(obj_pitch)) + obj_y*(cos(obj_yaw)*cos(obj_roll) + sin(obj_yaw)*sin(obj_roll)*sin(obj_pitch)) - fr1z*cos(obj_pitch)*sin(obj_yaw) + obj_z*cos(obj_pitch)*sin(obj_yaw))^2 + (fr1z*sin(obj_pitch) - obj_z*sin(obj_pitch) - fr1x*cos(obj_roll)*cos(obj_pitch) + obj_x*cos(obj_roll)*cos(obj_pitch) - fr1y*cos(obj_pitch)*sin(obj_roll) + obj_y*cos(obj_pitch)*sin(obj_roll))^2)^(1/2);
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               0];

% Tr11 = [right_normal1(2), -right_normal1(1),0];%cross(right_normal1,[0;0;1]);
% Tr11 = Tr11/norm(Tr11);
% Tr12 = cross(right_normal1,Tr11);

x = right_normal1(1); y = right_normal1(2); z = right_normal1(3);
Tr11 = [y,-x,0]';
Tr12 = [(x*z)/(abs(x)^2 + abs(y)^2)^(1/2);
     (y*z)/(abs(x)^2 + abs(y)^2)^(1/2);
     - x^2/(abs(x)^2 + abs(y)^2)^(1/2) - y^2/(abs(x)^2 + abs(y)^2)^(1/2)];
 
normal = right_normal1;
normal = R_world_to_B*normal;

d = R_world_to_B*[Tr11, Tr12];
%d = 0;
mu = 0;
end