AnimFSM = {
    current = "Idle",
    
    Update = function(self, dt)
        -- self.current = "Contents/Enemy/Pistol_Idle.fbx"
        self.current = "Contents/Enemy/Pistol_Idle"
        return {
            anim = self.current,
            -- blend = 5.0
        }
    end
}

return AnimFSM
