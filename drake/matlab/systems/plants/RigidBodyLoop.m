classdef RigidBodyLoop < RigidBodyElement
    
    properties
        name
        frameA
        frameB
        axis=[1;0;0];
        b = 0;      % added by NEEL
        k = 0;      % added by NEEL
        constraint_id
    end
    
    methods
        function [obj,model] = updateConstraints(obj,model)
            
            % todo: support planar kinematics here (should output only 2
            % constraints instead of 3)
            
            relative_position_fun = drakeFunction.kinematic.RelativePosition(model,obj.frameA,obj.frameB,zeros(3,1));
            %      relative_position_fun = relative_position_fun.addInputFrame(model.getVelocityFrame);
            position_constraint = DrakeFunctionConstraint(zeros(3,1),zeros(3,1),relative_position_fun);
            position_constraint.grad_level = 2;
            % todo: naming logic should go into the constraint classes
            % todo: support 2D constraints for planar loops?
            position_constraint = setName(position_constraint,{[obj.name,'_x'];[obj.name,'_y'];[obj.name,'_z']});
            
            % a second relative position constraint enforces the orientation
            relative_position_fun = drakeFunction.kinematic.RelativePosition(model,obj.frameA,obj.frameB,obj.axis);
            %      relative_position_fun = relative_position_fun.addInputFrame(model.getVelocityFrame);
            orientation_constraint = DrakeFunctionConstraint(obj.axis,obj.axis,relative_position_fun);
            orientation_constraint.grad_level = 2;
            orientation_constraint = setName(orientation_constraint,{[obj.name,'_ax'];[obj.name,'_ay'];[obj.name,'_az']});
            
            if isempty(obj.constraint_id)
                [model,obj.constraint_id(1)] = addPositionEqualityConstraint(model,position_constraint);
                [model,obj.constraint_id(2)] = addPositionEqualityConstraint(model,orientation_constraint);
            else
                model = updatePositionEqualityConstraint(model,obj.constraint_id(1),position_constraint);
                model = updatePositionEqualityConstraint(model,obj.constraint_id(2),orientation_constraint);
            end
        end
        
        % ADDED BY NEEL (5/10/17): computes spatial forces from
        % k/damping about axis of loop joint (frameA = parent)
        function [f_ext,df_ext] = computeSpatialForce(obj,manip,q,qd)
            
            parent_body = manip.getFrame(obj.frameA).body_ind;
            child_body = manip.getFrame(obj.frameB).body_ind;
            
            kinopt.base_or_frame_id = parent_body;      % first link in chain
            kinopt.rotation_type = 1;                   % we want euler angles
            
            nq = size(q, 1);
            if (obj.b~=0)
                if (nargout>1)
                    kinsol = doKinematics(manip,q,qd,struct('compute_gradients', true));
                    [x1to2, J1to2, dJ1to2] = manip.forwardKin(kinsol,child_body,zeros(3,1), kinopt);
                    J1to2dot = reshape(reshape(dJ1to2, 6*nq, nq)*qd, 6, nq);
                    v1to2 = J1to2*qd;
                    dv1to2_dq = J1to2dot;
                    dv1to2_dqd = J1to2;
                else
                    kinsol = doKinematics(manip,q);
                    [x1to2,J1to2] = manip.forwardKin(kinsol,child_body,zeros(3,1), kinopt);
                    v1to2 = J1to2*qd;
                end
            else
                kinsol = doKinematics(manip,q);
                if (nargout >1)
                    [x1to2, J1to2] = manip.forwardKin(kinsol,child_body,zeros(3,1), kinopt);
                    
                    v1to2 = 0;
                    dv1to2_dq = zeros(1,nq);
                    dv1to2_dqd = zeros(1,nq);
                    
                else
                    [x1to2, J1to2] = manip.forwardKin(kinsol,child_body,zeros(3,1), kinopt);
                    v1to2 = 0;
                end
            end
            axis_ind = find(obj.axis ~=0)+3;
            if isempty(axis_ind)
                error('Manipulator Dynamics: Neels implementation failed');
            end
            
            % scalar torque ( i know this is dumb ...)
            torque = -obj.k*x1to2(axis_ind) - obj.b*v1to2(axis_ind);
            f_ext = sparse(6,getNumBodies(manip));
            
            if (nargout>1)
                dtht_dq = J1to2(axis_ind, :);
                dthtd_dq = dv1to2_dq(axis_ind,:);
                dtorque_dq = -obj.k*dtht_dq - obj.b*dthtd_dq;
                
                dthtd_dqd = dv1to2_dqd(axis_ind,:);
                dtorque_dqd = -obj.b*dthtd_dqd;
                
                df_ext = sparse(6*getNumBodies(manip),size(q,1)+size(qd,1));
            end
            
            wrench_on_child_in_child_frame = [obj.axis*torque;zeros(3,1)];            
            f_ext(:,child_body) =  wrench_on_child_in_child_frame;
            
            if parent_body ~= 0 % don't apply force to world body
                T_parent_to_child_joint_predecessor = homogTransInv(manip.body(child_body).Ttree);
                AdT_parent_to_child_joint_predecessor = transformAdjoint(T_parent_to_child_joint_predecessor);
                f_ext(:,parent_body) = -AdT_parent_to_child_joint_predecessor' * wrench_on_child_in_child_frame;
            end
            if (nargout>1)
                df_ext((child_body-1)*6+1:child_body*6,1:size([q; qd],1)) = [[obj.axis*dtorque_dq; ...
                    zeros(3,size(q,1))], [obj.axis*dtorque_dqd; zeros(3,size(q,1))] ];
                if parent_body ~= 0
                    df_ext((parent_body-1)*6+1:parent_body*6,1:size([q; qd],1)) = -AdT_parent_to_child_joint_predecessor' * ...
                        [[obj.axis*dtorque_dq; zeros(3,size(q,1))], [obj.axis*dtorque_dqd; zeros(3,size(q,1))]];
                end
                df_ext = reshape(df_ext,6,[]);
            end
        end
        
    end
    
    methods (Static)
        function [model,loop] = parseURDFNode(model,robotnum,node,options)
            loop = RigidBodyLoop();
            loop.name = char(node.getAttribute('name'));
            loop.name = regexprep(loop.name, '\.', '_', 'preservecase');
            
            link1Node = node.getElementsByTagName('link1').item(0);
            body = findLinkId(model,char(link1Node.getAttribute('link')),robotnum);
            xyz = zeros(3,1);
            if link1Node.hasAttribute('xyz')
                xyz = reshape(str2num(char(link1Node.getAttribute('xyz'))),3,1);
            end
            rpy=[0;0;0];  % default according to URDF documentation
            if link1Node.hasAttribute('rpy')
                rpy = reshape(str2num(char(link1Node.getAttribute('rpy'))),3,1);
            end
            [model,loop.frameA] = addFrame(model,RigidBodyFrame(body,xyz,rpy,[loop.name,'FrameA']));
            
            link2Node = node.getElementsByTagName('link2').item(0);
            body = findLinkId(model,char(link2Node.getAttribute('link')),robotnum);
            xyz = zeros(3,1);
            if link2Node.hasAttribute('xyz')
                xyz = reshape(str2num(char(link2Node.getAttribute('xyz'))),3,1);
            end
            rpy=[0;0;0];  % default according to URDF documentation
            if link2Node.hasAttribute('rpy')
                rpy = reshape(str2num(char(link2Node.getAttribute('rpy'))),3,1);
            end
            [model,loop.frameB] = addFrame(model,RigidBodyFrame(body,xyz,rpy,[loop.name,'FrameB']));
            
            axisnode = node.getElementsByTagName('axis').item(0);
            if ~isempty(axisnode)
                if axisnode.hasAttribute('xyz')
                    axis = reshape(parseParamString(model,robotnum,char(axisnode.getAttribute('xyz'))),3,1);
                    axis = axis/(norm(axis)+eps); % normalize
                end
            end
            loop.axis = axis;
                         
            % ADDED BY NEEL (5/10/17) -- allows a loop joint to have a
            % stiffness and damping about its axis (loop.axis);
            dynamics = node.getElementsByTagName('dynamics').item(0);
            if ~isempty(dynamics)
                if dynamics.hasAttribute('damping')
                    damping = parseParamString(model,robotnum,char(dynamics.getAttribute('damping')));
                    if damping < 0
                        error('RigidBodyManipulator: damping coefficient must be >= 0');
                    end
                end
                loop.b = damping;
                if dynamics.hasAttribute('stiffness')
                    stiffness = parseParamString(model,robotnum,char(dynamics.getAttribute('stiffness')));
                    if stiffness < 0
                        error('RigidBodyManipulator: stiffness coefficient must be >= 0');
                    end
                end
                loop.k = stiffness;
            end
            
            type = char(node.getAttribute('type'));
            if ~strcmpi(type,'continuous')
                error(['joint type ',type,' not supported (yet?)']);
            end
        end
    end
end
