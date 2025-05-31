AnimFSM = {
    current = "Idle",
    Update = function(self, dt)

        -- self.current = "Contents/Fbx/Capoeira.fbx"

        local Exciting = self.Owner.Exciting

        if Exciting < 2 then
            self.current = "Contents/Fbx/Capoeira.fbx"
        else
            self.current = "Contents/Fbx/Twerkbin.fbx"
        end

        self.current = "Contents/Fbx/Twerkbin.fbx"

        return {
            anim = self.current,
            blend = 10.0
        }
    end
}

return AnimFSM