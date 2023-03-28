
class Engine : public PlatformListener
{
public:
    Engine(Platform *platform);
private:
    Platform *m_platform;
}

Engine::Engine(Platform *platform) :
    m_platform{platform}
{
    platform->add_listener(this);
}
