/*
 *  SPDX-FileCopyrightText: 2007-2008 Cyrille Berger <cberger@cberger.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef _KO_COLOR_CONVERSION_SYSTEM_H_
#define _KO_COLOR_CONVERSION_SYSTEM_H_

class KoColorProfile;
struct KoColorProfileQuery;
class KoColorSpace;
class KoColorSpaceFactory;
class KoColorSpaceEngine;
class KoID;

#include "KoColorConversionTransformation.h"

#include <QList>
#include <QPair>

#include "kritapigment_export.h"

/**
 * This class hold the logic related to pigment's Color Conversion System. It's
 * basically a graph containing all the possible color transformation between
 * the color spaces. The most useful functions are createColorConverter to create
 * a color conversion between two color spaces, and insertColorSpace which is called
 * by KoColorSpaceRegistry each time a new color space is added to the registry.
 *
 * This class is not part of public API, and can be changed without notice.
 */
class KRITAPIGMENT_EXPORT KoColorConversionSystem
{
public:
    struct RegistryInterface {
        virtual ~RegistryInterface() {}

        virtual const KoColorSpace * colorSpace(const QString & colorModelId, const QString & colorDepthId, const QString &profileName) = 0;
        virtual const KoColorSpace *colorSpace(const QString &colorModelId, const QString &colorDepthId, const KoColorProfile *profile) = 0;
        virtual const KoColorProfile *profileByName(const QString &name) const = 0;
        virtual const KoColorSpaceFactory* colorSpaceFactory(const QString &colorModelId, const QString &colorDepthId) const = 0;
        virtual QList<const KoColorProfile *>  profilesFor(const KoColorSpaceFactory * csf) const = 0;
        virtual QList<const KoColorSpaceFactory*> colorSpacesFor(const KoColorProfile* profile) const = 0;
    };

public:
    struct Node;
    struct Vertex;
    struct NodeKey;
    friend uint qHash(const KoColorConversionSystem::NodeKey &key);
    struct Path;
    /**
     * Construct a Color Conversion System, leave to the KoColorSpaceRegistry to
     * create it.
     */
    KoColorConversionSystem(RegistryInterface *registryInterface);
    ~KoColorConversionSystem();

    /**
     * Add a color space to a graph of transformation. The new node will be
     * added for each known compatible profile.
     *
     * Make sure you call `requiredConnectionProfilesFor(csf)` and add all
     * the required connection profiles into the registry **before** inserting
     * the actual color space \p csf. Otherwise the created node will not be
     * able to connect itself into the graph (due to a missing connection
     * point).
     *
     * \see requiredConnectionProfilesFor()
     */
    void insertColorSpace(const KoColorSpaceFactory *csf);

    /**
     * Add a profile to the graph of transformation. The new node will be added
     * for each known color space compatible with this profile.
     *
     * Make sure you call `requiredConnectionProfilesFor(profile)` and add all
     * the required connection profiles into the registry **before** inserting
     * the actual profile \p profile. Otherwise the created node will not be
     * able to connect itself into the graph (due to a missing connection
     * point).
     *
     * \see requiredConnectionProfilesFor()
     */
    void insertColorProfile(const KoColorProfile *profile);

    /**
     * \return a list of connection profiles required for this color space
     *
     * Some color space nodes may require custom profiles to be connected
     * to the color conversion system. I.e. Display P3 PQ space will require
     * a Display P3 Linear profile to connect itself to the color conversion
     * system through.
     *
     * This function returns a list of such connection profiles.
     *
     * Connection profiles should be requested and added to the registry
     * **before** adding the color space \p csf into the color conversion system
     * with insertColorSpace(csf).
     */
    QList<KoColorProfileQuery> requiredConnectionProfilesFor(const KoColorSpaceFactory* csf);

    /**
     * \return a list of connection profiles required for this profile
     *
     * Some color space nodes may require custom profiles to be connected
     * to the color conversion system. I.e. Display P3 PQ space will require
     * a Display P3 Linear profile to connect itself to the color conversion
     * system through.
     *
     * This function returns a list of such connection profiles.
     *
     * Connection profiles should be requested and added to the registry
     * **before** adding the profile \p profile into the color conversion system
     * with insertColorProfile(csf).
     */
    QList<KoColorProfileQuery> requiredConnectionProfilesFor(const KoColorProfile* profile);

    /**
     * This function is called by the color space to create a color conversion
     * between two color space. This function search in the graph of transformations
     * the best possible path between the two color space.
     */
    KoColorConversionTransformation* createColorConverter(const KoColorSpace * srcColorSpace, const KoColorSpace * dstColorSpace, KoColorConversionTransformation::Intent renderingIntent, KoColorConversionTransformation::ConversionFlags conversionFlags) const;

    /**
     * This function creates two transformations, one from the color space and one to the
     * color space. The destination color space is picked from a list of color space, such
     * as the conversion between the two color space is of the best quality.
     *
     * The typical use case of this function is for KoColorTransformationFactory which
     * doesn't support all color spaces, so unsupported color space have to find an
     * acceptable conversion in order to use that KoColorTransformationFactory.
     *
     * @param colorSpace the source color space
     * @param possibilities a list of color space among which we need to find the best
     *                      conversion
     * @param fromCS the conversion from the source color space will be affected to this
     *               variable
     * @param toCS the revert conversion to the source color space will be affected to this
     *             variable
     */
    void createColorConverters(const KoColorSpace* colorSpace, const QList< QPair<KoID, KoID> >& possibilities, KoColorConversionTransformation*& fromCS, KoColorConversionTransformation*& toCS) const;
public:
    /**
     * This function return a text that can be compiled using dot to display
     * the graph of color conversion connection.
     */
    QString toDot() const;
    /**
     * This function return a text that can be compiled using dot to display
     * the graph of color conversion connection, with a red link to show the
     * path of the best color conversion.
     */
    QString bestPathToDot(const QString& srcKey, const QString& dstKey) const;
public:
    /**
     * @return true if there is a path between two color spaces
     */
    bool existsPath(const QString& srcModelId, const QString& srcDepthId, const QString& srcProfileName, const QString& dstModelId, const QString& dstDepthId, const QString& dstProfileName) const;
    /**
     * @return true if there is a good path between two color spaces
     */
    bool existsGoodPath(const QString& srcModelId, const QString& srcDepthId, const QString& srcProfileName, const QString& dstModelId, const QString& dstDepthId, const QString& dstProfileName) const;

    /**
     * @return the best path for the specified color spaces. Used for
     * testing purposes only
     */
    Path findBestPath(const QString& srcModelId, const QString& srcDepthId, const QString& srcProfileName, const QString& dstModelId, const QString& dstDepthId, const QString& dstProfileName) const;

    /**
     * @return the best path for the specified color spaces. Used for
     * testing purposes only
     */
    Path findBestPath(const NodeKey &src, const NodeKey &dst) const;
private:
    QString vertexToDot(Vertex* v, const QString &options) const;
private:
    /**
     * Insert an engine.
     */
    Node* insertEngine(const KoColorSpaceEngine* engine);
    KoColorConversionTransformation* createTransformationFromPath(const KoColorConversionSystem::Path& path, const KoColorSpace* srcColorSpace, const KoColorSpace* dstColorSpace, KoColorConversionTransformation::Intent renderingIntent, KoColorConversionTransformation::ConversionFlags conversionFlags) const;
    /**
     * Query the registry to get the color space associated with this
     * node. (default profile)
     */
    const KoColorSpace* defaultColorSpaceForNode(const Node* node) const;
    /**
     * Create a new node
     */
    Node *createNode(const NodeKey &key, const KoColorProfile *profile = nullptr, const QByteArray &data = {});
    /**
     * Initialise a node for ICC color spaces
     */
    void connectToEngine(Node* _node, Node* _engine);
    const Node* nodeFor(const KoColorSpace*) const;
    /**
     * @return the node corresponding to that key, or create it if needed
     */
    Node* nodeFor(const NodeKey& key);
    Node *nodeFor(const NodeKey &key, const QByteArray &profileData);
    const Node* nodeFor(const NodeKey& key) const;
    /**
     * @return the list of nodes that correspond to a given model and depth.
     */
    QList<Node*> nodesFor(const QString& _modelId, const QString& _depthId);
    /**
     * @return the node associated with that key, and create it if needed
     */
    Node* nodeFor(const QString& colorModelId, const QString& colorDepthId, const QString& _profileName);
    Node *nodeFor(const QString &colorModelId, const QString &colorDepthId, const KoColorProfile *profile);
    const Node* nodeFor(const QString& colorModelId, const QString& colorDepthId, const QString& _profileName) const;
    /**
     * @return the vertex between two nodes, or null if the vertex doesn't exist
     */
    Vertex* vertexBetween(Node* srcNode, Node* dstNode);
    /**
     * create a vertex between two nodes and return it.
     */
    Vertex* createVertex(Node* srcNode, Node* dstNode);
    /**
     * looks for the best path between two nodes
     */
    Path findBestPath(const Node* srcNode, const Node* dstNode) const;
    /**
     * Delete all the paths of the list given in argument.
     */
    void deletePaths(QList<KoColorConversionSystem::Path*> paths) const;

private:
    struct Private;
    Private* const d;
};

#endif
