AnimFSM = {
    current = "Idle",
    
    Update = function(self, dt)
        -- self.current = "Contents/Enemy/Pistol_Idle.fbx"
        -- print(self.this.State)
        print(self.Owner.State)

        local S = self.Owner.State

        -- 패트롤
        if S == 0 then
            self.current = "Contents/Enemy/mixamo.com"
            self.current = "Contents/Walking/mixamo.com"
        else
            self.current = "Contents/Walking/mixamo.com"
            self.current = "Contents/Enemy/mixamo.com"
        end

        return {
            anim = self.current,
            blend = 1.0
        }
    end
}

return AnimFSM
